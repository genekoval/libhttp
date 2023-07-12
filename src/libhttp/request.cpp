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
}

namespace http {
    request::request() :
        handle(curl_easy_init()),
        response_stream(handle)
    {
        if (!handle) throw client_error("failed to create curl easy handle");

        set(CURLOPT_READFUNCTION, read_callback);
        set(CURLOPT_WRITEFUNCTION, write_callback);
    }

    request::request(request&& other) :
        body_data(std::move(other.body_data)),
        handle(std::exchange(other.handle, nullptr)),
        response_stream(std::move(other.response_stream)),
        header_list(std::exchange(other.header_list, {})),
        url_data(std::exchange(other.url_data, {})),
        current_method(other.current_method)
    {}

    request::~request() {
        curl_easy_cleanup(handle);
    }

    auto request::operator=(request&& other) -> request& {
        body_data = std::move(other.body_data);
        handle = std::exchange(other.handle, nullptr);
        response_stream = std::move(other.response_stream);
        header_list = std::exchange(other.header_list, {});
        url_data = std::move(other.url_data);
        current_method = other.current_method;

        return *this;
    }

    auto request::body(std::string_view data) -> void {
        body_data.data = data;
        set(CURLOPT_POSTFIELDS, nullptr); // Get data from the read callback
    }

    auto request::collect() -> ext::jtask<std::string> {
        auto stream = this->stream();
        co_return co_await stream.collect();
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

    auto request::perform() -> http::response {
        TIMBER_TRACE("{} starting blocking transfer", *this);

        TIMBER_TIMER(
            fmt::format("{} blocking transfer took", *this),
            timber::level::trace
        );

        pre_perform();

        const auto code = curl_easy_perform(handle);

        post_perform(code);

        return http::response(handle);
    }

    auto request::perform(http::client& client) -> ext::task<http::response> {
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

        co_return http::response(handle);
    }

    auto request::pre_perform() -> void {
        set(CURLOPT_READDATA, &body_data);
        set(CURLOPT_WRITEDATA, this);
    }

    auto request::post_perform(CURLcode code) -> void {
        body_data.written = 0;
        response_stream.end();
        response_stream = http::stream(handle);

        if (code != CURLE_OK) {
            throw client_error("curl: ({}) {}", code, curl_easy_strerror(code));
        }
    }

    auto request::response() const noexcept -> http::response {
        return http::response(handle);
    }

    auto request::stream() -> readable_stream {
        return response_stream;
    }

    auto request::url() -> http::url& {
        set(CURLOPT_CURLU, url_data.data());
        return url_data;
    }

    auto request::url(std::string_view url) -> void {
        set(CURLOPT_URL, url.data());
    }

    auto request::write_callback(
        char* ptr,
        std::size_t size,
        std::size_t nmemb,
        void* userdata
    ) -> std::size_t {
        auto& request = *reinterpret_cast<http::request*>(userdata);
        auto& stream = request.response_stream;

        if (stream.awaiting()) {
            TIMBER_DEBUG("{} read incoming bytes: {:L}", request, nmemb);

            stream.write(std::span<std::byte> {
                reinterpret_cast<std::byte*>(ptr),
                nmemb
            });

            return nmemb;
        }

        TIMBER_DEBUG("{} paused");

        stream.paused = true;
        return CURL_WRITEFUNC_PAUSE;
    }
}
