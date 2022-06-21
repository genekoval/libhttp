#pragma once

#include <curl/curl.h>
#include <fmt/core.h>
#include <string>

namespace http {
    class url final {
        CURLU* handle;
        char* part = nullptr;

        auto check_return_code(CURLUcode code) -> void;
    public:
        url();

        url(const url& other);

        ~url();

        auto data() -> CURLU*;

        auto get(CURLUPart what, unsigned int flags = 0) -> std::string_view;

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
