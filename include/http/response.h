#pragma once

#include "stream.hpp"

#include <curl/curl.h>
#include <ext/coroutine>
#include <span>
#include <string_view>

namespace http {
    struct status {
        const long code = 0;

        status() = default;

        status(long code);

        operator long() const noexcept;

        auto ok() const noexcept -> bool;
    };

    class response {
        CURL* handle;
    public:
        response(CURL* handle);

        auto content_length() const noexcept -> long;

        auto ok() const noexcept -> bool;

        auto status() const noexcept -> http::status;
    };
}
