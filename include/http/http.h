#pragma once

#include <curl/curl.h>
#include <string>

namespace http {
    enum {
        GET
    };

    struct options {
        int method;
    };

    class response {
        CURL* handle;

        template <typename T>
        auto get(CURLINFO info, T t) -> void {
            curl_easy_getinfo(handle, info, t);
        }
    public:
        response(CURL* handle);
        ~response();

        auto ok() -> bool;

        auto status() -> long;
    };

    struct client {
        client();
        ~client();

        auto request(const std::string& url) const -> response;

        auto request(const std::string& url, options&& opts) const -> response;
    };
}
