#pragma once

#include <http/http.h>

#include <curl/curl.h>
#include <fmt/format.h>
#include <string>

namespace http::internal {
    class header_list {
        curl_slist* list = nullptr;

        auto add(const char* item) -> void;
    public:
        ~header_list();

        auto add(std::string_view key, std::string_view value) -> void;

        auto data() -> curl_slist*;

        auto empty() const -> bool;
    };

    struct body_data {
        std::string data;
        std::size_t written = 0;
    };

    class request {
        body_data m_body;
        CURL* handle;
        header_list headers;

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

        auto header(std::string_view key, std::string_view value) -> void;

        auto method(int mtd) -> void;

        auto perform() -> response;

        auto url(const std::string& url_string) -> void;
    };
}
