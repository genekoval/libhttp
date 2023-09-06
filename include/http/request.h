#pragma once

#include "stream.hpp"
#include "url.h"

#include <http/error.h>
#include <http/response.h>
#include <http/session.hpp>
#include <timber/timber>

namespace http {
    struct file_deleter {
#ifndef NDEBUG
        CURL* handle;
        std::filesystem::path path;
#endif
        auto operator()(FILE* file) const noexcept -> void;
    };

    using file_stream = std::unique_ptr<FILE, file_deleter>;

    struct file {
        file_stream stream;
        std::size_t size;
    };

    class request {
        friend class session;
        friend class method_guard;

        friend struct fmt::formatter<request>;

        static auto write_stream(
            char* ptr,
            std::size_t size,
            std::size_t nmemb,
            void* userdata
        ) noexcept -> std::size_t;

        static auto write_string(
            char* ptr,
            std::size_t size,
            std::size_t nmemb,
            void* userdata
        ) noexcept -> std::size_t;

        CURL* handle;
        curl_slist* headers = nullptr;
        std::variant<std::monostate, std::string, std::string_view, file> body;
        std::variant<std::string, file, FILE*, http::stream> response_data;

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

        auto post_perform(
            CURLcode code,
            std::exception_ptr exception
        ) -> http::response;
    public:
        using header_type = std::pair<std::string_view, std::string_view>;

        std::string_view method = "GET";
        http::url url;

        request();

        request(const request&) = delete;

        request(request&& other);

        ~request();

        auto operator=(const request&) -> request& = delete;

        auto operator=(request&& other) -> request&;

        auto content_type(const media_type& type) -> void;

        auto data(std::string&& data) -> void;

        auto data_view(std::string_view data) -> void;

        auto download(const std::filesystem::path& location) -> void;

        auto follow_redirects(bool enable) -> void;

        template <typename T>
        auto header(std::string_view name, const T& value) -> void {
            const auto item = fmt::format("{}: {}", name, value);
            headers = curl_slist_append(headers, item.data());
        }

        auto perform() -> http::response;

        auto perform(http::session& session) -> ext::task<http::response>;

        auto pipe() -> readable_stream;

        auto pipe(FILE* file) -> void;

        auto upload(const std::filesystem::path& file) -> void;
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
