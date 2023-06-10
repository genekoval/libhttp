#pragma once

#include "request.hpp"
#include "response.hpp"

#include <fmt/format.h>
#include <memory>

namespace http::server {
    namespace detail {
        template <std::size_t... I>
        auto extract(std::index_sequence<I...>) {
            return std::tuple(I...);
        }
    }

    class session;

    class stream {
        stream* next = this;
        stream* prev = this;
        std::unique_ptr<const char[]> path_storage;

        auto process_path(std::string_view path) -> void;

        auto process_query(
            std::string_view query,
            char* buffer,
            std::size_t pos
        ) -> void;

        auto unlink() noexcept -> void;
    public:
        const std::int32_t id;

        bool active = false;
        bool open = true;

        server::request request;
        server::response response;

        stream();

        stream(std::int32_t id);

        stream(const stream&) = delete;

        stream(stream&&) = delete;

        ~stream();

        auto operator=(const stream&) -> stream& = delete;

        auto operator=(stream&&) -> stream& = delete;

        auto delete_all() noexcept -> void;

        auto link(stream& other) noexcept -> void;

        auto recv_header(std::string_view name, std::string_view value) -> void;
    };
}

template <>
struct fmt::formatter<http::server::stream> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const http::server::stream& stream, FormatContext& ctx) {
        auto buffer = fmt::memory_buffer();
        auto out = std::back_inserter(buffer);

        format_to(
            out,
            "Stream ID: {}, Scheme: {}, Authority: {}\n{} {}",
            stream.id,
            stream.request.scheme,
            stream.request.authority,
            stream.request.method,
            stream.request.path
        );

        if (!stream.request.query.empty()) {
            format_to(out, "\nQuery ({}):", stream.request.query.size());

            for (const auto& entry : stream.request.query) {
                format_to(out, "\n\t{} = {}", entry.first, entry.second);
            }
        }

        if (!stream.request.headers.empty()) {
            format_to(out, "\nHeaders ({}):", stream.request.headers.size());

            for (const auto& entry : stream.request.headers) {
                format_to(out, "\n\t{}: {}", entry.first, entry.second);
            }
        }

        return formatter<std::string_view>::format(
            {buffer.data(), buffer.size()},
            ctx
        );
    }
};
