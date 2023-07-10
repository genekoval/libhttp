#include <http/stream.hpp>

#include <cassert>

namespace http {
    stream::stream(CURL* handle) : handle(handle) {}

    auto stream::awaiting() const noexcept -> bool {
        return coroutine != nullptr;
    }

    auto stream::await_ready() const noexcept -> bool {
        return eof;
    }

    auto stream::await_suspend(
        std::coroutine_handle<> coroutine
    ) noexcept -> void {
        this->coroutine = coroutine;

        if (paused) {
            paused = false;
            curl_easy_pause(handle, CURLPAUSE_CONT);
        }
    }

    auto stream::await_resume() noexcept -> std::span<std::byte> {
        coroutine = nullptr;
        return data;
    }

    auto stream::end() noexcept -> void {
        eof = true;
        data = std::span<std::byte>();
        if (coroutine) coroutine.resume();
    }

    auto stream::write(std::span<std::byte> data) -> void {
        this->data = data;
        coroutine.resume();
    }

    readable_stream::readable_stream(stream& source) : source(&source) {}

    readable_stream::readable_stream(readable_stream&& other) :
        source(std::exchange(other.source, nullptr))
    {}

    auto readable_stream::operator=(
        readable_stream&& other
    ) -> readable_stream& {
        source = std::exchange(other.source, nullptr);
        return *this;
    }

    auto readable_stream::collect() -> ext::task<std::string> {
        auto result = std::string();
        auto chunk = std::span<std::byte>();

        do {
            chunk = co_await read();

            result.append(
                reinterpret_cast<const char*>(chunk.data()),
                chunk.size()
            );
        } while (!chunk.empty());

        co_return result;
    }

    auto readable_stream::read() -> ext::task<std::span<std::byte>> {
        assert(source);
        co_return co_await *source;
    }
}
