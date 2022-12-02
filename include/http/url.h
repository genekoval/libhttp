#pragma once

#include <http/string.h>

#include <curl/curl.h>
#include <fmt/core.h>
#include <string>

namespace http {
    class url final {
        CURLU* handle;

        auto check_return_code(CURLUcode code) -> void;
    public:
        url();

        url(const url& other);

        url(url&& other);

        ~url();

        auto operator=(const url& other) -> url&;

        auto operator=(url&& other) -> url&;

        template <typename T>
        auto append_query(std::string_view key, const T& value) -> void {
            set(
                CURLUPART_QUERY,
                fmt::format("{}={}", key, value),
                CURLU_APPENDQUERY
            );
        }

        auto clear(CURLUPart part) -> void;

        auto data() -> CURLU*;

        auto get(CURLUPart what, unsigned int flags = 0) -> string;

        template <typename... Args>
        auto path(std::string_view format_string, Args&&... args) -> void {
            set(CURLUPART_PATH, fmt::format(
                fmt::runtime(format_string),
                std::forward<Args>(args)...
            ));
        }

        auto set(
            CURLUPart part,
            const char* content,
            unsigned int flags = 0
        ) -> void;

        auto set(
            CURLUPart part,
            const std::string& content,
            unsigned int flags = 0
        ) -> void;
    };
}
