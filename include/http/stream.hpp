#pragma once

#include <coroutine>
#include <curl/curl.h>
#include <ext/coroutine>
#include <fmt/format.h>
#include <span>

namespace http {
    class stream {
        friend class fmt::formatter<stream>;

        CURL* handle = nullptr;
        std::coroutine_handle<> coroutine;
        std::span<std::byte> data;
        bool eof = false;
    public:
        bool aborted = false;
        bool paused = false;

        stream() = default;

        explicit stream(CURL* handle);

        stream(const stream&) = delete;

        stream(stream&& other);

        ~stream();

        auto operator=(const stream&) -> stream& = delete;

        auto operator=(stream&& other) -> stream&;

        auto awaiting() const noexcept -> bool;

        auto await_ready() const noexcept -> bool;

        auto await_suspend(
            std::coroutine_handle<> coroutine
        ) noexcept -> void;

        auto await_resume() noexcept -> std::span<std::byte>;

        auto close() -> void;

        auto end() noexcept -> void;

        auto expected_size() const -> long;

        auto write(std::span<std::byte> data) -> void;
    };

    class readable_stream {
        stream* source = nullptr;
    public:
        readable_stream() = default;

        readable_stream(stream& source);

        readable_stream(const readable_stream&) = delete;

        readable_stream(readable_stream&& other);

        ~readable_stream();

        auto operator=(const readable_stream&) -> readable_stream& = delete;

        auto operator=(readable_stream&& other) -> readable_stream&;

        auto expected_size() const noexcept -> long;

        auto read() -> ext::task<std::span<std::byte>>;
    };
}

namespace fmt {
    template <>
    struct formatter<http::stream> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const http::stream& stream, FormatContext& ctx) {
            return fmt::format_to(ctx.out(), "request ({})", ptr(stream.handle));
        }
    };
}
