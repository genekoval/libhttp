#pragma once

#include <http/server/request.hpp>

namespace http::server::extractor {
    class stream {
        server::request& request;
    public:
        stream(server::request& request);

        auto read() -> ext::task<std::span<const std::byte>>;
    };
}
