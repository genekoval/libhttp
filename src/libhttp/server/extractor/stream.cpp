#include <http/server/extractor/stream.hpp>

namespace http::server::extractor {
    stream::stream(server::request& request) : request(request) {}

    auto stream::read() -> ext::task<std::span<const std::byte>> {
        if (!request.eof) co_await request.continuation;
        co_return request.data;
    }
}
