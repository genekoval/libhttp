#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace http {
    using json = nlohmann::json;

    class response {
        std::string buffer;
        CURL* handle;
    public:
        response(CURL* handle, std::string&& buffer);

        auto json() -> http::json;

        auto length() -> long;

        auto ok() -> bool;

        auto status() -> long;

        auto text() -> std::string;
    };
}
