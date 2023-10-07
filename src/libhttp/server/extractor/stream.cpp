#include <http/server/extractor/stream.hpp>

namespace http::server::extractor {
    stream::stream(server::request& request) : request(&request) {}

    stream::stream(stream&& other) :
        request(std::exchange(other.request, nullptr)) {}

    stream::~stream() {
        if (!request) return;

        request->discard = true;
        request->continuation.resume();
    }

    auto stream::operator=(stream&& other) -> stream& {
        if (std::addressof(other) != this) {
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
        }

        return *this;
    }

    auto stream::read() -> ext::task<std::span<const std::byte>> {
        if (!request->eof && request->data.empty()) {
            co_await request->continuation;
        }

        co_return std::exchange(request->data, {});
    }
}
