#pragma once

#include <coroutine>
#include <curl/curl.h>
#include <ext/coroutine>
#include <span>

namespace http {
    class stream {
        CURL* handle;
        std::coroutine_handle<> coroutine;
        std::span<std::byte> data;
        bool eof = false;
    public:
        bool paused = false;

        explicit stream(CURL* handle);

        auto awaiting() const noexcept -> bool;

        auto await_ready() const noexcept -> bool;

        auto await_suspend(
            std::coroutine_handle<> coroutine
        ) noexcept -> void;

        auto await_resume() noexcept -> std::span<std::byte>;

        auto end() noexcept -> void;

        auto write(std::span<std::byte> data) -> void;
    };

    class readable_stream {
        stream* source = nullptr;
    public:
        readable_stream() = default;

        readable_stream(stream& source);

        readable_stream(const readable_stream&) = delete;

        readable_stream(readable_stream&& other);

        auto operator=(const readable_stream&) -> readable_stream& = delete;

        auto operator=(readable_stream&& other) -> readable_stream&;

        auto collect() -> ext::task<std::string>;

        auto read() -> ext::task<std::span<std::byte>>;
    };
}
