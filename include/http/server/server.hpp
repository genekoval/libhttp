#pragma once

#include "router.hpp"
#include "session.hpp"

#include <netcore/netcore>

namespace http::server {
    class context {
        const std::size_t buffer_size;
        server::router* router;
        session sessions;
    public:
        context();

        context(server::router& router);

        context(server::router& router, std::size_t buffer_size);

        auto connection(netcore::ssl::socket&& client) -> ext::task<>;

        auto shutdown() -> void;
    };

    using ssl_context = netcore::ssl::server<context>;
    using server = netcore::server<ssl_context>;
    using server_list = netcore::server_list<ssl_context>;
}
