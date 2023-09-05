#include <http/request.h>

#include <ext/scope>

namespace fs = std::filesystem;

namespace {
    using http::file;
    using http::file_deleter;
    using http::file_stream;
    using http::request;

    template <typename... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <typename... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    auto open(const fs::path& path, CURL* handle) -> file {
        if (auto* stream = std::fopen(path.c_str(), "r")) {
            TIMBER_DEBUG(
                R"(request ({}) opened file "{}")",
                fmt::ptr(handle),
                path.native()
            );

#ifndef NDEBUG
            return file {
                .stream = file_stream(stream, file_deleter {
                    .handle = handle,
                    .path = path
                }),
                .size = fs::file_size(path)
            };
#else
            return file {
                .stream = file_stream(file),
                .size = fs::file_size(path)
            };
#endif
        }

        TIMBER_DEBUG(
            R"(request ({}) failed to open file "{}")",
            fmt::ptr(handle),
            path.native()
        );

        throw ext::system_error(fmt::format(
            R"(Failed to open file "{}")",
            path.native()
        ));
    }
}

namespace http {
    auto file_deleter::operator()(FILE* file) const noexcept -> void {
        if (!file) return;

        if (fclose(file) == 0) {
#ifndef NDEBUG
            TIMBER_DEBUG(
                R"(request ({}) closed file "{}")",
                fmt::ptr(handle),
                path.native()
            );
#endif
        }
        else {
            const auto error = std::error_code(errno, std::generic_category());

#ifndef NDEBUG
            TIMBER_ERROR(
                R"(request ({}) failed to close file stream "{}": {})",
                fmt::ptr(handle),
                path.native(),
                error.message()
            );
#else
            TIMBER_ERROR(
                R"(Failed to close file stream "{}": {})",
                path.native(),
                error.message()
            );
#endif
        }
    }

    request::request() : handle(curl_easy_init()) {
        if (!handle) throw client_error("Failed to create curl easy handle");
    }

    request::request(request&& other) :
        handle(std::exchange(other.handle, nullptr)),
        headers(std::exchange(other.headers, nullptr)),
        body(std::exchange(other.body, {})),
        response_data(std::move(other.response_data)),
        method(std::exchange(other.method, "GET")),
        url(std::move(other.url))
    {}

    request::~request() {
        curl_easy_cleanup(handle);
        curl_slist_free_all(headers);
    }

    auto request::operator=(request&& other) -> request& {
        if (std::addressof(other) != this) {
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
        }

        return *this;
    }

    auto request::content_type(const media_type& type) -> void {
        header("content-type", type);
    }

    auto request::data(std::string_view data) -> void {
        body = data;
    }

    auto request::download(const fs::path& location) -> void {
        response_data = open(location, handle);
    }

    auto request::follow_redirects(bool enable) -> void {
        set(CURLOPT_FOLLOWLOCATION, enable);
    }

    auto request::perform() -> http::response {
        TIMBER_TRACE("{} starting blocking transfer", *this);

        TIMBER_TIMER(
            fmt::format("{} blocking transfer took", *this),
            timber::level::trace
        );

        pre_perform();

        const auto code = curl_easy_perform(handle);

        return post_perform(code, nullptr);
    }

    auto request::perform(http::session& session) -> ext::task<http::response> {
        TIMBER_TRACE(
            "{} starting nonblocking transfer using {}",
            *this,
            session
        );

        TIMBER_TIMER(
            fmt::format("{} nonblocking transfer took", *this),
            timber::level::trace
        );

        pre_perform();

        CURLcode code = CURLE_OK;
        auto exception = std::exception_ptr();

        try {
            code = co_await session.perform(handle);
        }
        catch (...) {
            exception = std::current_exception();
        }

        co_return post_perform(code, exception);
    }

    auto request::pre_perform() -> void {
        set(CURLOPT_CUSTOMREQUEST, method.data());
        set(CURLOPT_CURLU, url.data());
        set(CURLOPT_HTTPHEADER, headers);

        std::visit(overloaded {
            [this](std::monostate) {
                if (method == "HEAD") set(CURLOPT_NOBODY, 1L);
                else set(CURLOPT_HTTPGET, 1L);
            },
            [this](std::string_view string) {
                set(CURLOPT_POST, 1L);
                set(CURLOPT_POSTFIELDS, string.data());
                set(CURLOPT_POSTFIELDSIZE_LARGE, string.size());
            },
            [this](const file& file) {
                set(CURLOPT_UPLOAD, 1L);
                set(CURLOPT_INFILESIZE_LARGE, file.size);
                set(CURLOPT_READFUNCTION, nullptr);
                set(CURLOPT_READDATA, file.stream.get());
            }
        }, body);

        std::visit(overloaded {
            [this](const std::string& string) {
                set(CURLOPT_WRITEFUNCTION, write_string);
                set(CURLOPT_WRITEDATA, &string);
            },
            [this](const file& file) {
                set(CURLOPT_WRITEFUNCTION, nullptr);
                set(CURLOPT_WRITEDATA, file.stream.get());
            },
            [this](FILE* file) {
                set(CURLOPT_WRITEFUNCTION, nullptr);
                set(CURLOPT_WRITEDATA, file);
            },
            [this](const http::stream& stream) {
                set(CURLOPT_WRITEFUNCTION, write_stream);
                set(CURLOPT_WRITEDATA, &stream);
            }
        }, response_data);
    }

    auto request::post_perform(
        CURLcode code,
        std::exception_ptr exception
    ) -> http::response {
        const auto deferred = ext::scope_exit([this] { response_data = {}; });

        body = {};

        if (exception) std::rethrow_exception(exception);

        if (code != CURLE_OK) {
            throw client_error("curl: ({}) {}", code, curl_easy_strerror(code));
        }

        auto data = std::string();

        if (auto* const string = std::get_if<std::string>(&response_data)) {
            data = std::move(*string);
        }

        auto res = http::response(handle, std::move(data));

        TIMBER_DEBUG("{} {} {}", res.status(), method, url);

        return res;
    }

    auto request::pipe() -> readable_stream {
        return response_data.emplace<stream>(handle);
    }

    auto request::pipe(FILE* file) -> void {
        response_data = file;
    }

    auto request::upload(const fs::path& file) -> void {
        body = open(file, handle);
    }

    auto request::write_stream(
        char* ptr,
        std::size_t size,
        std::size_t nmemb,
        void* userdata
    ) noexcept -> std::size_t {
        auto& stream = *reinterpret_cast<http::stream*>(userdata);

        if (stream.aborted) return CURL_WRITEFUNC_ERROR;

        if (stream.awaiting()) {
            const auto real_size = size * nmemb;

            TIMBER_DEBUG(
                "{} read {:L} byte{}",
                stream,
                real_size,
                real_size == 1 ? "" : "s"
            );

            stream.write(std::span<std::byte> {
                reinterpret_cast<std::byte*>(ptr),
                real_size
            });

            return real_size;
        }

        TIMBER_DEBUG("{} paused", stream);

        stream.paused = true;
        return CURL_WRITEFUNC_PAUSE;
    }

    auto request::write_string(
        char* ptr,
        std::size_t size,
        std::size_t nmemb,
        void* userdata
    ) noexcept -> std::size_t {
        const auto real_size = size * nmemb;
        auto& string = *reinterpret_cast<std::string*>(userdata);

        string.append(ptr, real_size);

        return real_size;
    }
}
