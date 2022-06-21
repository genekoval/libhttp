#pragma once

#include <curl/curl.h>
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
