#pragma once

#include <curl/curl.h>

namespace http {
    class response {
        CURL* handle;
        long response_code;
    public:
        response(CURL* handle);

        auto ok() -> bool;

        auto status() -> long;
    };
}
