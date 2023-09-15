#pragma once

#include <http/server/request.hpp>

namespace http::server::extractor {
    class stream {
        server::request* request;
    public:
        stream(server::request& request);

        stream(const stream&) = delete;

        stream(stream&& other);

        ~stream();

        auto operator=(const stream&) -> stream& = delete;

        auto operator=(stream&& other) -> stream&;

        auto read() -> ext::task<std::span<const std::byte>>;
    };
}
