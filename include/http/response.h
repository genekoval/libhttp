#pragma once

#include "stream.hpp"

#include <curl/curl.h>
#include <ext/coroutine>
#include <span>
#include <string_view>

namespace http {
    class response {
        long response_code;
    public:
        response(CURL* handle);

        auto ok() const noexcept -> bool;

        auto status() const noexcept -> long;
    };
}
