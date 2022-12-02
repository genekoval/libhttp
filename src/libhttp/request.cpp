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
        auto& buffer = *reinterpret_cast<std::string*>(userdata);
        buffer.append(ptr, nmemb);
        return nmemb;
    }
}

namespace http {
    request::request() : handle(curl_easy_init()) {
        if (!handle) throw client_error("failed to create curl easy handle");

        set(CURLOPT_CURLU, url_data.data());
        set(CURLOPT_READFUNCTION, read_callback);
        set(CURLOPT_WRITEFUNCTION, write_callback);
    }

    request::request(request&& other) :
        body_data(std::move(other.body_data)),
        buffer(std::move(other.buffer)),
        handle(std::exchange(other.handle, nullptr)),
        header_list(std::exchange(other.header_list, {})),
        url_data(std::exchange(other.url_data, {})),
        current_method(other.current_method)
    {}

    request::~request() {
        curl_easy_cleanup(handle);
    }

    auto request::operator=(request&& other) -> request& {
        body_data = std::move(other.body_data);
        buffer = std::move(other.buffer);
        handle = std::exchange(other.handle, nullptr);
        header_list = std::exchange(other.header_list, {});
        url_data = std::move(other.url_data);
        current_method = other.current_method;

        return *this;
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

        if (option != current_method) {
            set(current_method, 0L);
            set(option, 1L);

            current_method = option;
        }
    }

    auto request::method(
        http::method method,
        std::string_view custom
    ) -> method_guard {
        this->method(method);
        set(CURLOPT_CUSTOMREQUEST, custom.data());

        return method_guard(this);
    }

    auto request::perform() -> response {
        TIMBER_TRACE("{} starting blocking transfer", *this);

        TIMBER_TIMER(
            fmt::format("{} blocking transfer took", *this),
            timber::level::trace
        );

        pre_perform();

        const auto code = curl_easy_perform(handle);

        post_perform(code);

        return response(handle, buffer);
    }

    auto request::perform(http::client& client) -> ext::task<response> {
        TIMBER_TRACE(
            "{} starting nonblocking transfer using {}",
            *this,
            client
        );

        TIMBER_TIMER(
            fmt::format("{} nonblocking transfer took", *this),
            timber::level::trace
        );

        pre_perform();

        const auto code = co_await client.perform(handle);

        post_perform(code);

        co_return response(handle, buffer);
    }

    auto request::pre_perform() -> void {
        buffer.clear();

        set(CURLOPT_READDATA, &body_data);
        set(CURLOPT_WRITEDATA, &buffer);
    }

    auto request::post_perform(CURLcode code) -> void {
        body_data.written = 0;

        if (code != CURLE_OK) {
            throw client_error("curl: ({}) {}", code, curl_easy_strerror(code));
        }
    }

    auto request::url() -> http::url& {
        return url_data;
    }
}
