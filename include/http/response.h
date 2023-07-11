#pragma once

#include "stream.hpp"

#include <curl/curl.h>
#include <ext/coroutine>
#include <span>
#include <string_view>

namespace http {
    class response {
        CURL* handle;
    public:
        response(CURL* handle);

        auto content_length() const noexcept -> long;

        auto ok() const noexcept -> bool;

        auto status() const noexcept -> long;
    };
}
