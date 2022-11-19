#pragma once

#include <curl/curl.h>
#include <string_view>

namespace http {
    class response {
        std::string_view body_data;
        CURL* handle;
        long response_code;
    public:
        response(CURL* handle, std::string_view body);

        auto body() const noexcept -> std::string_view;

        auto ok() const noexcept -> bool;

        auto status() const noexcept -> long;
    };
}
