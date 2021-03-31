#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>

namespace http {
    enum {
        GET,
        HEAD,
        PUT,
        DELETE
    };

    struct options {
        std::string body;
        std::vector<std::pair<std::string, std::string>> headers;
        int method;
    };

    class response {
        std::string buffer;
        CURL* handle;

        template <typename T>
        auto get(CURLINFO info, T t) -> void {
            curl_easy_getinfo(handle, info, t);
        }
    public:
        response(CURL* handle, std::string&& buffer);
        ~response();

        auto json() -> nlohmann::json;

        auto length() -> long;

        auto ok() -> bool;

        auto status() -> long;

        auto text() -> std::string;
    };

    struct client {
        client();
        ~client();

        auto request(const std::string& url) const -> response;

        auto request(const std::string& url, options&& opts) const -> response;
    };
}
