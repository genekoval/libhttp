#include <http/stream.hpp>

#include <cassert>
#include <cstring>
#include <fstream>

namespace http {
    stream::stream(CURL* handle) : handle(handle) {}

    stream::stream(stream&& other) :
        handle(std::exchange(other.handle, nullptr)),
        coroutine(std::exchange(other.coroutine, nullptr)),
        data(std::exchange(other.data, {})),
        eof(std::exchange(other.eof, false)),
        paused(std::exchange(other.paused, false)) {}

    stream::~stream() {
        eof = true;
        data = {};

        if (coroutine) coroutine.resume();
    }

    auto stream::operator=(stream&& other) -> stream& {
        if (std::addressof(other) != this) {
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
        }

        return *this;
    }

    auto stream::awaiting() const noexcept -> bool {
        return coroutine != nullptr;
    }

    auto stream::await_ready() const noexcept -> bool { return eof; }

    auto stream::await_suspend(std::coroutine_handle<> coroutine) noexcept
        -> void {
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

    auto stream::close() -> void {
        aborted = true;
        coroutine = nullptr;

        if (paused) {
            paused = false;
            curl_easy_pause(handle, CURLPAUSE_CONT);
        }
    }

    auto stream::end() noexcept -> void {
        eof = true;
        data = std::span<std::byte>();
        if (coroutine) coroutine.resume();
    }

    auto stream::expected_size() const -> long {
        curl_off_t result = 0;
        curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &result);
        return result;
    }

    auto stream::write(std::span<std::byte> data) -> void {
        this->data = data;
        coroutine.resume();
    }

    readable_stream::readable_stream(stream& source) : source(&source) {}

    readable_stream::readable_stream(readable_stream&& other) :
        source(std::exchange(other.source, nullptr)) {}

    readable_stream::~readable_stream() {
        if (source) source->close();
    }

    auto readable_stream::operator=(readable_stream&& other)
        -> readable_stream& {
        source = std::exchange(other.source, nullptr);
        return *this;
    }

    auto readable_stream::expected_size() const noexcept -> long {
        return source->expected_size();
    }

    auto readable_stream::read() -> ext::task<std::span<std::byte>> {
        assert(source);
        co_return co_await *source;
    }
}
