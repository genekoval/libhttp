#pragma once

#include <http/error.h>

#include <curl/curl.h>
#include <ext/coroutine>
#include <netcore/netcore>

namespace http {
    class client final {
        friend struct fmt::formatter<client>;

        class socket {
            netcore::system_event& event;
        public:
            std::optional<int> updates;

            socket(netcore::system_event& event);

            auto notify() -> void;
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
            std::reference_wrapper<netcore::event<CURLcode>>
        > handles;
        netcore::event<> messages;
        ext::jtask<> message_task;
        int running_handles = 0;
        netcore::timer timer;

        auto action(curl_socket_t sockfd, int ev_bitmask = 0) -> void;

        auto add(CURL* handle, netcore::event<CURLcode>& event) -> void;

        auto assign(curl_socket_t fd, socket& sock) -> void;

        auto cleanup() const noexcept -> void;

        auto manage_socket(
            curl_socket_t sockfd,
            uint32_t events
        ) -> ext::detached_task;

        auto manage_timer(
            std::chrono::milliseconds timeout
        ) -> ext::detached_task;

        auto read_info() -> void;

        auto remove(CURL* handle) -> netcore::event<CURLcode>&;

        auto set(CURLMoption option, auto value) -> void {
            const auto code = curl_multi_setopt(handle, option, value);

            if (code != CURLM_OK) throw client_error(
                "failed to set curl option ({}): {}",
                option,
                curl_multi_strerror(code)
            );
        }

        auto wait_for_messages() -> ext::jtask<>;
    public:
        client();

        client(const client&) = delete;

        client(client&&) = delete;

        ~client();

        auto operator=(const client&) -> client& = delete;

        auto operator=(client&&) -> client& = delete;

        auto perform(CURL* req) -> ext::task<CURLcode>;
    };
}

namespace fmt {
    template <>
    struct formatter<http::client> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const http::client& client, FormatContext& ctx) {
            return format_to(ctx.out(), "client ({})", ptr(client.handle));
        }
    };
}
