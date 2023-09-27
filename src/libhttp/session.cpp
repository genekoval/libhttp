#include <http/request.h>
#include <http/session.hpp>

#include <cassert>
#include <fmt/chrono.h>
#include <sys/epoll.h>
#include <timber/timber>

using std::chrono::milliseconds;

namespace fmt {
    template <>
    struct formatter<http::session::socket> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const http::session::socket& socket, FormatContext& ctx) {
            return fmt::format_to(ctx.out(), "curl socket ({})", socket.fd());
        }
    };
}

namespace http {
    session::socket::socket(curl_socket_t sockfd) :
        event(netcore::runtime::event::create(sockfd, EPOLLIN | EPOLLOUT))
    {}

    auto session::socket::fd() const noexcept -> curl_socket_t {
        return event->fd();
    }

    auto session::socket::remove() const noexcept -> void {
        if (const auto error = event->remove()) {
            TIMBER_ERROR(
                "Failed to remove file descriptor from runtime: {}",
                error.message()
            );
        }

        event->cancel();
    }

    auto session::socket::update(int what) noexcept -> bool {
        switch (what) {
            case CURL_POLL_IN:
                TIMBER_TRACE("{} wants to read", *this);
                event->events = EPOLLIN;
                break;
            case CURL_POLL_OUT:
                TIMBER_TRACE("{} wants to write", *this);
                event->events = EPOLLOUT;
                break;
            case CURL_POLL_INOUT:
                TIMBER_TRACE("{} wants to read and write", *this);
                event->events = EPOLLIN | EPOLLOUT;
                break;
            case CURL_POLL_REMOVE:
                TIMBER_TRACE("{} removal requested", *this);
                remove();
                break;
            default:
                TIMBER_ERROR("Unexpected curl socket status ({})", what);
                return false;
        }

        return true;
    }

    auto session::socket::wait() -> ext::task<int> {
        const auto events = co_await event->out();
        int result = 0;

        if (events & EPOLLIN && this->event->events & EPOLLIN) {
            result |= CURL_CSELECT_IN;
            TIMBER_TRACE("{} ready to read", *this);
        }

        if (events & EPOLLOUT && this->event->events & EPOLLOUT) {
            result |= CURL_CSELECT_OUT;
            TIMBER_TRACE("{} ready to write", *this);
        }

        co_return result;
    }

    auto session::socket_callback(
        CURL* easy,
        curl_socket_t sockfd,
        int what,
        void* userp,
        void* socketp
    ) -> int {
        auto& client = *static_cast<http::session*>(userp);

        TIMBER_TRACE("{} socket ({}) callback", client, sockfd);

        if (socketp) {
            auto& socket = *static_cast<http::session::socket*>(socketp);
            TIMBER_TRACE("{} update requested", socket);
            if (!socket.update(what)) return -1;
        }
        else {
            auto success = false;
            client.manage_socket(sockfd, what, success);
            if (!success) return -1;

            TIMBER_TRACE(
                "{} socket ({}) created for request ({})",
                client,
                sockfd,
                fmt::ptr(easy)
            );
        }

        return 0;
    }

    auto session::timer_callback(
        CURLM* multi,
        long timeout_ms,
        void* userp
    ) -> int {
        const auto timeout = milliseconds(timeout_ms);
        auto& client = *static_cast<http::session*>(userp);

        TIMBER_TRACE("{} timer callback ({:L})", client, timeout);

        client.timeout(timeout_ms);

        if (timeout_ms > -1) client.timer.set(timeout);
        if (timeout_ms <= 0 && client.timer.waiting()) client.timer.disarm();

        return 0;
    }

    session::session() :
        handle(curl_multi_init()),
        timer(netcore::timer::monotonic()),
        timer_task(manage_timer())
    {
        if (!handle) throw client_error("failed to create curl multi handle");

        TIMBER_TRACE("{} created", *this);

        set(CURLMOPT_SOCKETFUNCTION, socket_callback);
        set(CURLMOPT_SOCKETDATA, this);

        set(CURLMOPT_TIMERFUNCTION, timer_callback);
        set(CURLMOPT_TIMERDATA, this);
    }

    session::~session() {
        cleanup();

        TIMBER_TRACE("{} destroyed", *this);
    }

    auto session::action(curl_socket_t sockfd, int ev_bitmask) -> void {
        TIMBER_TRACE(
            "{} curl socket {} action",
            *this,
            sockfd == CURL_SOCKET_TIMEOUT ?
                "timeout" : fmt::format("({})", sockfd)
        );

        const auto code = curl_multi_socket_action(
            handle,
            sockfd,
            ev_bitmask,
            &running_handles
        );

        if (code != CURLM_OK) throw client_error(
            "failed to perform curl socket action: {}",
            curl_multi_strerror(code)
        );

        TIMBER_TRACE(
            "{} curl socket {} action complete: {:L} running handle{}",
            *this,
            sockfd == CURL_SOCKET_TIMEOUT ?
                "timeout" : fmt::format("({})", sockfd),
            running_handles,
            running_handles == 1 ? "" : "s"
        );

        read_info();
    }

    auto session::add(
        CURL* easy_handle,
        ext::continuation<CURLcode>& continuation
    ) -> void {
        const auto code = curl_multi_add_handle(handle, easy_handle);

        if (code != CURLM_OK) throw client_error(
            "failed to add curl handle to client ({}): {}",
            code,
            curl_multi_strerror(code)
        );

        handles.insert({easy_handle, continuation});
    }

    auto session::assign(socket& sock) -> bool {
        const auto code = curl_multi_assign(handle, sock.fd(), &sock);

        if (code != CURLM_OK) {
            TIMBER_ERROR(
                "Failed to assign socket data pointer to fd: {}",
                curl_multi_strerror(code)
            );

            return false;
        }

        return true;
    }

    auto session::cleanup() const noexcept -> void {
        const auto code = curl_multi_cleanup(handle);

        if (code == CURLM_OK) return;

        TIMBER_ERROR(
            "{} error during client cleanup ({}): {}",
            *this,
            code,
            curl_multi_strerror(code)
        );
    }

    auto session::manage_socket(
        socket socket,
        int what,
        bool& success
    ) -> ext::detached_task {
        if (!(success = assign(socket) && socket.update(what))) co_return;

        while (const auto events = co_await socket.wait()) {
            action(socket.fd(), events);
        }

        TIMBER_TRACE("{} task complete", socket);
    }

    auto session::manage_timer() -> ext::jtask<> {
        while (true) {
            const auto timeout = co_await this->timeout;

            switch (timeout) {
                case -1:
                    continue;
                case 0:
                    co_await netcore::yield();
                    break;
                default:
                    if (!co_await timer.wait()) continue;
                    break;
            }

            action(CURL_SOCKET_TIMEOUT);
        }
    }

    auto session::perform(CURL* easy_handle) -> ext::task<CURLcode> {
        auto transfer_complete = ext::continuation<CURLcode>();

        add(easy_handle, transfer_complete);

        co_return co_await transfer_complete;
    }

    auto session::read_info() -> void {
        TIMBER_TRACE("{} checking for messages", *this);

        CURLMsg* message = nullptr;

        do {
            int queue = 0;

            message = curl_multi_info_read(handle, &queue);
            if (!message) continue;

            if (message->msg == CURLMSG_DONE) {
                auto* const handle = message->easy_handle;
                const auto code = message->data.result;

                auto& transfer_complete = remove(handle);
                TIMBER_TRACE(
                    "request ({}) transfer complete",
                    fmt::ptr(handle)
                );

                transfer_complete.resume(code);
            }

            TIMBER_TRACE(
                "{} {:L} additional message{} in queue",
                *this,
                queue,
                queue == 1 ? "" : "s"
            );
        } while (message);
    }

    auto session::remove(CURL* easy_handle) -> ext::continuation<CURLcode>& {
        const auto code =  curl_multi_remove_handle(handle, easy_handle);

        if (code != CURLM_OK) {
            TIMBER_ERROR(
                "failed to remove curl handle from client ({}): {}",
                code,
                curl_multi_strerror(code)
            );
        }

        const auto result = handles.find(easy_handle);
        assert(result != handles.end());

        const auto transfer_complete = result->second;
        handles.erase(result);

        return transfer_complete;
    }
}
