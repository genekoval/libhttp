#include <http/request.h>

namespace {
    auto read_callback(
        char* buffer,
        size_t size,
        size_t nitems,
        void* userdata
    ) -> size_t {
        auto* body = reinterpret_cast<http::body_data*>(userdata);
        auto data = body->data;

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
        auto* memory = reinterpret_cast<http::memory*>(userdata);
        memory->append(ptr, nmemb);
        return nmemb;
    }
}

namespace http {
    auto memory::append(const char* ptr, std::size_t size) -> void {
        const auto required = storage.size() + padding + size;

        if (storage.capacity() < required) {
            storage.reserve(required);
        }

        storage.append(ptr, size);
    }

    request::request() : handle(curl_easy_init()) {
        if (!handle) {
            throw http::client_error("failed to create curl handle");
        }

        set(CURLOPT_CURLU, url_data.data());
        set(CURLOPT_READFUNCTION, read_callback);
        set(CURLOPT_WRITEFUNCTION, write_callback);
    }

    request::~request() {
        curl_easy_cleanup(handle);
    }

    auto request::body(std::string_view data) -> void {
        body_data.data = data;
        set(CURLOPT_POSTFIELDS, nullptr); // Get data from the read callback
    }

    auto request::headers(std::initializer_list<header_type> headers) -> void {
        if (!header_list.empty()) header_list = http::header_list();

        for (const auto& [key, value] : headers) {
            header_list.add(key, value);
        }

        set(CURLOPT_HTTPHEADER, header_list.data());
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

    auto request::perform(http::memory& memory) -> response {
        memory.storage.clear();

        set(CURLOPT_READDATA, &body_data);
        set(CURLOPT_WRITEDATA, &memory);

        const auto code = curl_easy_perform(handle);
        body_data.written = 0;

        if (code != CURLE_OK) {
            throw client_error("curl: ({}) {}", code, curl_easy_strerror(code));
        }

        return response(handle);
    }

    auto request::url() -> http::url& {
        return url_data;
    }
}
