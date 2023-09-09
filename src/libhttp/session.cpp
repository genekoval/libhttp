#include <http/request.h>
#include <http/session.hpp>

#include <cassert>
#include <fmt/chrono.h>
#include <sys/epoll.h>
#include <timber/timber>

using std::chrono::milliseconds;

namespace http {
    session::socket::socket(curl_socket_t sockfd) :
        sockfd(sockfd),
        event(sockfd)
    {}

    auto session::socket::fd() const noexcept -> curl_socket_t {
        return sockfd;
    }

    auto session::socket::update(int what) -> bool {
        std::uint32_t events = 0;

        switch (what) {
            case CURL_POLL_IN:
                TIMBER_TRACE("socket ({}) wants to read", sockfd);
                events = EPOLLIN;
                break;
            case CURL_POLL_OUT:
                TIMBER_TRACE("socket ({}) wants to write", sockfd);
                events = EPOLLOUT;
                break;
            case CURL_POLL_INOUT:
                TIMBER_TRACE("socket ({}) wants to read and write", sockfd);
                events = EPOLLIN | EPOLLOUT;
                break;
            case CURL_POLL_REMOVE:
                TIMBER_TRACE("socket ({}) removal requested", sockfd);
                event.cancel();
                return true;
            default:
                TIMBER_ERROR("Unexpected curl socket status ({})", what);
                return false;
        }

        try {
            event.set(events);
            return true;
        }
        catch (const std::exception& ex) {
            TIMBER_ERROR("Failed to update curl socket events: {}", ex.what());
        }

        return false;
    }

    auto session::socket::wait() -> ext::task<int> {
        const auto events = co_await event.wait();
        int result = 0;

        if (events & EPOLLIN) {
            result |= CURL_CSELECT_IN;
            TIMBER_TRACE("socket ({}) ready to read", sockfd);
        }

        if (events & EPOLLOUT) {
            result |= CURL_CSELECT_OUT;
            TIMBER_TRACE("socket ({}) ready to write", sockfd);
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
            TIMBER_TRACE("socket ({}) update requested", sockfd);
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

        if (timeout_ms == -1) client.timer.disarm();
        else if (client.timer.waiting()) client.timer.set(timeout);
        else client.manage_timer(timeout);

        return 0;
    }

    session::session() :
        handle(curl_multi_init()),
        timer(netcore::timer::monotonic())
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

        while (true) {
            const auto events = co_await socket.wait();

            if (events == 0) {
                TIMBER_TRACE("socket ({}) task complete", socket.fd());
                co_return;
            }

            action(socket.fd(), events);
        }
    }

    auto session::manage_timer(milliseconds timeout) -> ext::detached_task {
        timer.set(timeout);

        if (timeout == milliseconds::zero()) co_await netcore::yield();
        else if (!co_await timer.wait()) co_return;

        action(CURL_SOCKET_TIMEOUT);
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
