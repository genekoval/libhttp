#include "request.h"

#include <array>
#include <fmt/format.h>
#include <stdexcept>

namespace http::internal {
    constexpr auto methods = std::array {
        CURLOPT_HTTPGET,
        CURLOPT_NOBODY,
        CURLOPT_UPLOAD
    };

    constexpr auto custom_methods = std::array<const char*, 1> {
        "DELETE"
    };

    static auto read_callback(
        char* buffer,
        size_t size,
        size_t nitems,
        void* userdata
    ) -> size_t {
        auto* body = reinterpret_cast<body_data*>(userdata);
        const auto& data = body->data;

        const auto max = size * nitems;
        const auto remaining = data.size() - body->written;
        const auto bytes = std::min(remaining, max);

        std::memcpy(buffer, &data[body->written], bytes);
        body->written += bytes;

        return bytes;
    }

    static auto write_callback(
        char* ptr,
        size_t size,
        size_t nmemb,
        void* userdata
    ) -> size_t {
        auto* buffer = reinterpret_cast<std::string*>(userdata);
        buffer->append(std::string_view(ptr, nmemb));

        return nmemb;
    }

    request::request() : handle(curl_easy_init()) {
        if (!handle) {
            throw std::runtime_error("failed to create cURL handle");
        }

        set(CURLOPT_READFUNCTION, read_callback);
        set(CURLOPT_WRITEFUNCTION, write_callback);
    }

    auto request::body(std::string&& data) -> void {
        m_body.data = std::move(data);
    }

    auto request::header(std::string_view key, std::string_view value) -> void {
        headers.add(key, value);
    }

    auto request::method(int mtd) -> void {
        if (static_cast<std::size_t>(mtd) < methods.size()) {
            set(methods[mtd], 1L);
        }
        else set(CURLOPT_CUSTOMREQUEST, custom_methods[mtd - methods.size()]);
    }

    auto request::perform() -> response {
        auto buffer = std::string();

        if (!headers.empty()) set(CURLOPT_HTTPHEADER, headers.data());
        set(CURLOPT_READDATA, &m_body);
        set(CURLOPT_WRITEDATA, &buffer);

        const auto code = curl_easy_perform(handle);

        if (code != CURLE_OK) {
            throw std::runtime_error(fmt::format(
                "cURL request failure: returned ({})",
                code
            ));
        }

        return response(handle, std::move(buffer));
    }

    auto request::url(const std::string& url_string) -> void {
        set(CURLOPT_URL, url_string.c_str());
    }
}
