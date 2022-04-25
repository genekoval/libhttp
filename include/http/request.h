#pragma once

#include <http/error.h>
#include <http/response.h>

namespace http {
    enum class method {
        GET,
        HEAD,
        POST,
        PUT
    };

    struct body_data {
        std::string data;
        std::size_t written = 0;
    };

    class header_list {
        curl_slist* list = nullptr;

        auto add(const char* item) -> void;
    public:
        ~header_list();

        auto add(std::string_view key, std::string_view value) -> void;

        auto data() -> curl_slist*;

        auto empty() const -> bool;
    };

    struct memory {
        std::string storage;
        std::size_t padding = 0;

        auto append(const char* ptr, std::size_t size) -> void;
    };

    class request {
        http::body_data body_data;
        CURL* handle;
        header_list headers;

        template <typename T>
        auto set(CURLoption option, T t) -> void {
            const auto result = curl_easy_setopt(handle, option, t);

            if (result != CURLE_OK) {
                throw http::client_error(
                    "failed to set curl option ({}): returned code ({})",
                    option,
                    result
                );
            }
        }
    public:
        request();

        ~request();

        auto body(std::string&& data) -> void;

        auto header(std::string_view key, std::string_view value) -> void;

        auto method(http::method method) -> void;

        auto method(std::string_view method) -> void;

        auto perform(http::memory& memory) -> response;

        auto url(std::string_view url_string) -> void;
    };
}
