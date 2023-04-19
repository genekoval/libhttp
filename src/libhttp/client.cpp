#include <http/client.hpp>
#include <http/request.h>

#include <cassert>
#include <fmt/chrono.h>
#include <sys/epoll.h>
#include <timber/timber>

using std::chrono::milliseconds;

namespace {
    auto translate(int what) -> uint32_t {
        uint32_t events = 0;

        switch (what) {
            case CURL_POLL_IN: events = EPOLLIN; break;
            case CURL_POLL_OUT: events = EPOLLOUT; break;
            case CURL_POLL_INOUT: events = EPOLLIN | EPOLLOUT; break;
        }

        return events;
    }
}

namespace http {
    client::socket::socket(netcore::system_event& event) : event(event) {}

    auto client::socket::notify() -> void {
        event.notify();
    }

    auto client::socket_callback(
        CURL* easy,
        curl_socket_t sockfd,
        int what,
        void* userp,
        void* socketp
    ) -> int {
        auto& client = *static_cast<http::client*>(userp);

        TIMBER_TRACE("{} socket callback ({})", client, sockfd);

        if (!socketp) {
            TIMBER_TRACE(
                "{} socket ({}) created for request ({})",
                client,
                sockfd,
                fmt::ptr(easy)
            );
            client.manage_socket(sockfd, translate(what));
            return 0;
        }

        TIMBER_TRACE("socket ({}) update requested", sockfd);

        auto& sock = *static_cast<socket*>(socketp);

        sock.updates = what;
        if (what == CURL_POLL_REMOVE) sock.notify();

        return 0;
    }

    auto client::timer_callback(
        CURLM* multi,
        long timeout_ms,
        void* userp
    ) -> int {
        auto& client = *static_cast<http::client*>(userp);

        TIMBER_TRACE("{} timer callback ({:L}ms)", client, timeout_ms);

        if (timeout_ms == -1) client.timer.disarm();
        else client.manage_timer(milliseconds(timeout_ms));

        return 0;
    }

    client::client() :
        handle(curl_multi_init()),
        message_task(wait_for_messages()),
        timer(netcore::timer::monotonic())
    {
        if (!handle) throw client_error("failed to create curl multi handle");

        TIMBER_TRACE("{} created", *this);

        set(CURLMOPT_SOCKETFUNCTION, socket_callback);
        set(CURLMOPT_SOCKETDATA, this);

        set(CURLMOPT_TIMERFUNCTION, timer_callback);
        set(CURLMOPT_TIMERDATA, this);
    }

    client::~client() {
        cleanup();

        TIMBER_TRACE("{} destroyed", *this);
    }

    auto client::action(curl_socket_t sockfd, int ev_bitmask) -> void {
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

        messages.emit();
    }

    auto client::add(CURL* handle, netcore::event<CURLcode>& event) -> void {
        const auto code = curl_multi_add_handle(this->handle, handle);

        if (code != CURLM_OK) throw client_error(
            "failed to add curl handle to client ({}): {}",
            code,
            curl_multi_strerror(code)
        );

        handles.insert({handle, event});
    }

    auto client::assign(curl_socket_t fd, socket& sock) -> void {
        const auto code = curl_multi_assign(handle, fd, &sock);

        if (code != CURLM_OK) throw client_error(
            "failed to assign socket data pointer to fd: {}",
            curl_multi_strerror(code)
        );
    }

    auto client::cleanup() const noexcept -> void {
        const auto code = curl_multi_cleanup(handle);

        if (code == CURLM_OK) return;

        TIMBER_ERROR(
            "{} error during client cleanup ({}): {}",
            *this,
            code,
            curl_multi_strerror(code)
        );
    }

    auto client::manage_socket(
        curl_socket_t sockfd,
        uint32_t events
    ) -> ext::detached_task {
        auto event = netcore::system_event(sockfd, events);
        auto sock = socket(event);
        assign(sockfd, sock);

        while (true) {
            if (sock.updates) {
                const auto updates = *sock.updates;

                if (updates == CURL_POLL_REMOVE) {
                    TIMBER_TRACE("socket ({}) task complete", sockfd);
                    co_return;
                }

                events = translate(updates);

                sock.updates.reset();
            }

            try {
                co_await event.wait(events, nullptr);
            }
            catch (const netcore::task_canceled&) {
                co_return;
            }

            if (sock.updates) continue;

            const auto latest = event.latest_events();
            int ev = 0;

            if (latest & EPOLLIN) {
                ev |= CURL_CSELECT_IN;
                TIMBER_TRACE("socket ({}) received event: IN", sockfd);
            }
            if (latest & EPOLLOUT) {
                ev |= CURL_CSELECT_OUT;
                TIMBER_TRACE("socket ({}) received event: OUT", sockfd);
            }

            TIMBER_TRACE("socket ({}) performing action", sockfd);
            action(sockfd, ev);
        }
    }

    auto client::manage_timer(milliseconds timeout) -> ext::detached_task {
        timer.set(timeout);

        if (timeout == milliseconds::zero()) co_await netcore::yield();
        else {
            try {
                co_await timer.wait();
            }
            catch (const netcore::task_canceled&) {
                co_return;
            }
        }

        action(CURL_SOCKET_TIMEOUT);
    }

    auto client::read_info() -> void {
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

                transfer_complete.emit(code);
            }

            TIMBER_TRACE("{} {:L} additional messages in queue", *this, queue);
        } while (message);
    }

    auto client::remove(CURL* handle) -> netcore::event<CURLcode>& {
        const auto code =  curl_multi_remove_handle(this->handle, handle);

        if (code != CURLM_OK) {
            TIMBER_ERROR(
                "failed to remove curl handle from client ({}): {}",
                code,
                curl_multi_strerror(code)
            );
        }

        const auto result = handles.find(handle);
        assert(result != handles.end());

        auto& transfer_complete = result->second.get();
        handles.erase(result);

        return transfer_complete;
    }

    auto client::perform(CURL* req) -> ext::task<CURLcode> {
        auto transfer_complete = netcore::event<CURLcode>();

        add(req, transfer_complete);

        co_return co_await transfer_complete.listen();
    }

    auto client::wait_for_messages() -> ext::jtask<> {
        while (true) {
            TIMBER_TRACE("{} waiting for messages", *this);

            co_await messages.listen();

            read_info();
        }
    }
}
