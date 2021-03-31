#pragma once

#include <http/http.h>

#include <curl/curl.h>
#include <fmt/format.h>
#include <string>

namespace http::internal {
    struct body_data {
        std::string data;
        std::size_t written = 0;
    };

    class request {
        body_data m_body;
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

        auto body(std::string&& data) -> void;

        auto method(int mtd) -> void;

        auto perform() -> response;

        auto url(const std::string& url_string) -> void;
    };
}
