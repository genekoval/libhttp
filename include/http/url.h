#pragma once

#include <http/string.h>

#include <curl/curl.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace http {
    class url final {
        CURLU* handle;

        auto check_return_code(CURLUcode code) const -> void;

        auto get(CURLUPart what, unsigned int flags = 0) const -> http::string;

        auto set(
            CURLUPart part,
            const char* content,
            unsigned int flags = 0
        ) -> void;

        auto set(
            CURLUPart part,
            std::string_view content,
            unsigned int flags = 0
        ) -> void;

        auto try_get(
            CURLUPart what,
            CURLUcode none,
            unsigned int flags = 0
        ) const -> std::optional<http::string>;
    public:
        url();

        url(const char* str);

        url(std::string_view string);

        url(const url& other);

        url(url&& other);

        ~url();

        auto operator=(const char* str) -> url&;

        auto operator=(std::string_view string) -> url&;

        auto operator=(const url& other) -> url&;

        auto operator=(url&& other) -> url&;

        auto clear(CURLUPart part) -> void;

        auto data() -> CURLU*;

        auto fragment() const -> std::optional<http::string>;

        auto fragment(std::string_view value) -> void;

        auto host() const -> std::optional<http::string>;

        auto host(std::string_view value) -> void;

        auto password() const -> std::optional<http::string>;

        auto password(std::string_view value) -> void;

        auto path() const -> http::string;

        auto path(std::string_view value) -> void;

        template <typename... Args>
        requires (sizeof...(Args) > 0)
        auto path(
            fmt::format_string<Args...> format_string,
            Args&&... args
        ) -> void {
            path(fmt::format(format_string, std::forward<Args>(args)...));
        }

        template <typename... Args>
        auto path_components(Args&&... args) -> void {
            auto buffer = fmt::memory_buffer();
            (fmt::format_to(std::back_inserter(buffer), "/{}", args), ...);
            path(fmt::to_string(buffer));
        }

        auto port() const -> std::optional<int>;

        auto port(int value) -> void;

        auto port(std::string_view value) -> void;

        auto port_or_default() const -> int;

        auto query() const -> std::optional<http::string>;

        auto query(std::string_view value) -> void;

        template <typename T>
        auto query(std::string_view key, const T& value) -> void {
            set(
                CURLUPART_QUERY,
                fmt::format("{}={}", key, value),
                CURLU_APPENDQUERY
            );
        }

        auto scheme() const -> std::optional<http::string>;

        auto scheme(std::string_view value) -> void;

        auto string() const -> http::string;

        auto user() const -> std::optional<http::string>;

        auto user(std::string_view value) -> void;
    };

    auto from_json(const nlohmann::json& json, url& url) -> void;

    auto to_json(nlohmann::json& json, const url& url) -> void;
}

template <>
struct fmt::formatter<http::url> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const http::url& url, FormatContext& ctx) const {
        return formatter<std::string_view>::format(url.string(), ctx);
    }
};
