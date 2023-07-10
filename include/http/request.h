#pragma once

#include "stream.hpp"
#include "url.h"

#include <http/client.hpp>
#include <http/error.h>
#include <http/response.h>
#include <timber/timber>

namespace http {
    enum class method {
        GET,
        HEAD,
        POST,
        PUT
    };

    struct body_data {
        std::string_view data;
        std::size_t written = 0;
    };

    class header_list {
        curl_slist* list = nullptr;

        auto add(const char* item) -> void;
    public:
        ~header_list();

        auto add(std::string_view key, std::string_view value) -> void;

        auto data() -> curl_slist*;

        auto empty() const -> bool;
    };

    class request;

    class method_guard final {
        http::request* request;
    public:
        method_guard(http::request* request);

        ~method_guard();
    };

    class request {
        friend class client;
        friend class method_guard;

        friend struct fmt::formatter<request>;

        static auto write_callback(
            char* ptr,
            std::size_t size,
            std::size_t nmemb,
            void* userdata
        ) -> std::size_t;

        http::body_data body_data;
        CURL* handle;
        http::stream response_stream;
        http::header_list header_list;
        http::url url_data;
        CURLoption current_method = CURLOPT_HTTPGET;

        template <typename T>
        auto set(CURLoption option, T t) -> void {
            const auto result = curl_easy_setopt(handle, option, t);

            if (result != CURLE_OK) {
                throw http::client_error(
                    "failed to set curl option ({}): {}",
                    option,
                    curl_easy_strerror(result)
                );
            }
        }

        auto pre_perform() -> void;

        auto post_perform(CURLcode code) -> void;
    public:
        using header_type = std::pair<std::string_view, std::string_view>;

        request();

        request(const request&) = delete;

        request(request&& other);

        ~request();

        auto operator=(const request&) -> request& = delete;

        auto operator=(request&& other) -> request&;

        auto body(std::string_view data) -> void;

        auto collect() -> ext::jtask<std::string>;

        auto headers(std::initializer_list<header_type> headers) -> void;

        auto method(http::method method) -> void;

        [[nodiscard("return value's destructor resets custom method")]]
        auto method(
            http::method method,
            std::string_view custom
        ) -> method_guard;

        auto perform() -> response;

        auto perform(http::client& client) -> ext::task<response>;

        auto stream() -> readable_stream;

        auto url() -> http::url&;
    };
}

namespace fmt {
    template <>
    struct formatter<http::request> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const http::request& request, FormatContext& ctx) {
            return format_to(ctx.out(), "request ({})", ptr(request.handle));
        }
    };
}
