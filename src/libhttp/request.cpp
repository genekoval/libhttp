#include <http/request.h>

namespace {
    auto read_callback(
        char* buffer,
        size_t size,
        size_t nitems,
        void* userdata
    ) -> size_t {
        auto* body = reinterpret_cast<http::body_data*>(userdata);
        const auto& data = body->data;

        const auto max = size * nitems;
        const auto remaining = data.size() - body->written;
        const auto bytes = std::min(remaining, max);

        std::memcpy(buffer, &data[body->written], bytes);
        body->written += bytes;

        return bytes;
    }

    auto write_callback(
        char* ptr,
        size_t size,
        size_t nmemb,
        void* userdata
    ) -> size_t {
        auto* buffer = reinterpret_cast<std::string*>(userdata);
        buffer->append(std::string_view(ptr, nmemb));

        return nmemb;
    }
}

namespace http {
    request::request() : handle(curl_easy_init()) {
        if (!handle) {
            throw http::client_error("failed to create curl handle");
        }

        set(CURLOPT_READFUNCTION, read_callback);
        set(CURLOPT_WRITEFUNCTION, write_callback);
    }

    request::~request() {
        curl_easy_cleanup(handle);
    }

    auto request::body(std::string&& data) -> void {
        body_data.data = std::move(data);
        set(CURLOPT_POSTFIELDS, body_data.data.data());
    }

    auto request::header(std::string_view key, std::string_view value) -> void {
        headers.add(key, value);
    }

    auto request::method(http::method method) -> void {
        CURLoption option;

        switch (method) {
            case http::method::GET: option = CURLOPT_HTTPGET; break;
            case http::method::HEAD: option = CURLOPT_NOBODY; break;
            case http::method::POST: option = CURLOPT_POST; break;
            case http::method::PUT: option = CURLOPT_UPLOAD; break;
        }

        set(option, 1L);
    }

    auto request::method(std::string_view method) -> void {
        set(CURLOPT_CUSTOMREQUEST, method.data());
    }

    auto request::perform() -> response {
        auto buffer = std::string();

        if (!headers.empty()) set(CURLOPT_HTTPHEADER, headers.data());
        set(CURLOPT_READDATA, &body_data);
        set(CURLOPT_WRITEDATA, &buffer);

        const auto code = curl_easy_perform(handle);

        if (code != CURLE_OK) {
            throw client_error("curl: ({}) {}", code, curl_easy_strerror(code));
        }

        return response(handle, std::move(buffer));
    }

    auto request::url(std::string_view url_string) -> void {
        set(CURLOPT_URL, url_string.data());
    }
}
