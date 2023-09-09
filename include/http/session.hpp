#pragma once

#include <http/error.h>

#include <curl/curl.h>
#include <ext/coroutine>
#include <netcore/netcore>

namespace http {
    class session final {
        friend struct fmt::formatter<session>;

        class socket {
            curl_socket_t sockfd;
            netcore::system_event event;
        public:
            socket(curl_socket_t sockfd);

            [[nodiscard]]
            auto fd() const noexcept -> curl_socket_t;

            [[nodiscard]]
            auto update(int what) -> bool;

            auto wait() -> ext::task<int>;
        };

        static auto socket_callback(
            CURL* easy,
            curl_socket_t sockfd,
            int what,
            void* userp,
            void* socketp
        ) -> int;

        static auto timer_callback(
            CURLM* multi,
            long timeout_ms,
            void* userp
        ) -> int;

        CURLM* handle;
        std::unordered_map<
            CURL*,
            std::reference_wrapper<ext::continuation<CURLcode>>
        > handles;
        int running_handles = 0;
        netcore::timer timer;

        auto action(curl_socket_t sockfd, int ev_bitmask = 0) -> void;

        auto add(
            CURL* easy_handle,
            ext::continuation<CURLcode>& continuation
        ) -> void;

        [[nodiscard]]
        auto assign(socket& sock) -> bool;

        auto cleanup() const noexcept -> void;

        auto manage_socket(
            socket socket,
            int what,
            bool& success
        ) -> ext::detached_task;

        auto manage_timer(
            std::chrono::milliseconds timeout
        ) -> ext::detached_task;

        auto read_info() -> void;

        auto remove(CURL* easy_handle) -> ext::continuation<CURLcode>&;

        auto set(CURLMoption option, auto value) -> void {
            const auto code = curl_multi_setopt(handle, option, value);

            if (code != CURLM_OK) throw client_error(
                "failed to set curl option ({}): {}",
                option,
                curl_multi_strerror(code)
            );
        }
    public:
        session();

        session(const session&) = delete;

        session(session&&) = delete;

        ~session();

        auto operator=(const session&) -> session& = delete;

        auto operator=(session&&) -> session& = delete;

        auto perform(CURL* easy_handle) -> ext::task<CURLcode>;
    };
}

namespace fmt {
    template <>
    struct formatter<http::session> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const http::session& session, FormatContext& ctx) {
            return format_to(ctx.out(), "session ({})", ptr(session.handle));
        }
    };
}
