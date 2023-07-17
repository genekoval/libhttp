#pragma once

#include "router.hpp"

#include <netcore/netcore>
#include <nghttp2/nghttp2.h>

namespace http::server {
    class session {
        friend struct fmt::formatter<session>;

        session* next = this;
        session* prev = this;

        stream streams;
        nghttp2_session* handle = nullptr;
        netcore::ssl::buffered_socket socket;
        http::server::router* router = nullptr;
        ext::counter tasks;
        ext::continuation<> closed;

        auto await_close() -> ext::task<>;

        auto recv() -> ext::task<>;

        auto respond(stream& stream) -> ext::task<>;

        auto send() -> ext::task<>;

        auto send_server_connection_header() -> ext::task<>;

        auto unlink() noexcept -> void;
    public:
        ext::continuation<>* pause = nullptr;

        session() = default;

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

        auto close() noexcept -> void;

        auto handle_connection() -> ext::task<>;

        auto handle_request(stream& stream) -> ext::detached_task;

        auto link(session& other) noexcept -> void;

        auto make_stream(std::int32_t id) -> stream&;
    };
}

template <>
struct fmt::formatter<http::server::session> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const http::server::session& session, FormatContext& ctx) {
        auto buffer = memory_buffer();
        auto out = std::back_inserter(buffer);

        format_to(out, "HTTP Session ({})", ptr(session.handle));

        return formatter<std::string_view>::format(
            {buffer.data(), buffer.size()},
            ctx
        );
    }
};
