#pragma once

#include "router.hpp"

#include <netcore/netcore>

namespace http::server {
    class context {
        const std::size_t buffer_size;
        server::router* router;
    public:
        context();

        context(server::router& router);

        context(server::router& router, std::size_t buffer_size);

        auto close() -> void;

        auto connection(netcore::ssl::socket&& client) -> ext::task<>;

        auto listen(const netcore::address_type& addr) -> void;
    };

    using server = netcore::server<netcore::ssl::server<context>>;
}
