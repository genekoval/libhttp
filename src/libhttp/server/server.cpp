#include <http/server/server.hpp>
#include <http/server/session.hpp>

#include <ext/data_size.h>

using namespace ext::literals;

namespace http::server {
    context::context() : buffer_size(8_KiB), router(nullptr) {}

    context::context(http::server::router& router) : context(router, 8_KiB) {}

    context::context(http::server::router& router, std::size_t buffer_size) :
        buffer_size(buffer_size),
        router(&router) {}

    auto context::connection(netcore::ssl::socket&& client) -> ext::task<> {
        const auto protocol = co_await client.accept();

        if (protocol != "h2") {
            throw std::runtime_error("Protocol h2 not negotiated");
        }

        auto session = http::server::session(
            std::forward<netcore::ssl::socket>(client),
            buffer_size,
            *router
        );

        sessions.link(session);

        co_await session.handle_connection();
    }

    auto context::shutdown() -> void {
        TIMBER_DEBUG("HTTP server shutdown requested");
        sessions.close();
    }
}
