#pragma once

#include <http/http.h>

#include <curl/curl.h>
#include <fmt/format.h>
#include <string>

namespace http::internal {
    class request {
        CURL* handle;

        template <typename T>
        auto set(CURLoption opt, T t) -> void {
            const auto result = curl_easy_setopt(handle, opt, t);

            if (result != CURLE_OK) {
                throw std::runtime_error(fmt::format(
                    "failed to set cURL option ({}): returned code ({})",
                    opt,
                    result
                ));
            }
        }
    public:
        request();

        auto method(int mtd) -> void;

        auto perform() -> response;

        auto url(const std::string& url_string) -> void;
    };
}
