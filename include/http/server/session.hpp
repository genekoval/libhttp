#pragma once

#include "router.hpp"

#include <netcore/netcore>
#include <nghttp2/nghttp2.h>

namespace http::server {
    class session {
        stream streams;
        nghttp2_session* handle = nullptr;
        netcore::ssl::buffered_socket socket;
        http::server::router& router;
        ext::counter tasks;

        auto recv() -> ext::task<>;

        auto respond(stream& stream) -> ext::task<>;

        auto send() -> ext::task<>;

        auto send_server_connection_header() -> ext::task<>;
    public:
        ext::continuation<>* pause = nullptr;

        session(
            netcore::ssl::socket&& socket,
            std::size_t buffer_size,
            http::server::router& router
        );

        session(const session&) = delete;

        session(session&&) = delete;

        ~session();

        auto operator=(const session&) -> session& = delete;

        auto operator=(session&&) -> session& = delete;

        auto handle_connection() -> ext::task<>;

        auto handle_request(stream& stream) -> ext::detached_task;

        auto make_stream(std::int32_t id) -> stream&;
    };
}
