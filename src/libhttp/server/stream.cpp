#include <http/server/session.hpp>
#include <http/server/stream.hpp>

#include <ext/string.h>

using namespace std::literals;

namespace {
    namespace header {
        constexpr auto method = ":method"sv;
        constexpr auto path = ":path"sv;
        constexpr auto scheme = ":scheme"sv;
        constexpr auto authority = ":authority"sv;
    }

    auto hex_to_uint(char c) -> std::uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;

        return 0;
    }

    auto percent_decode(
        std::string_view value,
        char* buffer,
        std::size_t& pos
    ) -> std::string_view {
        const auto start = pos;

        if (value.size() <= 3) {
            value.copy(buffer + pos, value.size());
            pos += value.size() + 1;
            buffer[pos - 1] = '\0';
        }
        else {
            std::size_t i = 0;

            while (i < value.size() - 2) {
                if (
                    value[i] != '%' ||
                    !std::isxdigit(value[i + 1]) ||
                    !std::isxdigit(value[i + 2])
                ) {
                    buffer[pos++] = value[i++];
                    continue;
                }

                const auto n =
                    (hex_to_uint(value[i + 1]) << 4) +
                    hex_to_uint(value[i + 2]);

                buffer[pos++] = static_cast<char>(n);
                i += 3;
            }

            std::memcpy(&buffer[pos], &value[i], 2);
            buffer[pos + 2] = '\0';
            pos += 3;
        }

        return {buffer + start, pos - start - 1};
    }
}

namespace http::server {
    stream::stream() : id(-1) {}

    stream::stream(std::int32_t id) : id(id) {
        TIMBER_TRACE("Stream ID {} opened", id);
    }

    stream::~stream() {
        unlink();

        if (request.continuation) request.continuation.resume(
            std::make_exception_ptr(stream_aborted())
        );

        if (id != -1) {
            TIMBER_TRACE("Stream ID {} closed", id);
        }
    }

    auto stream::delete_all() noexcept -> void {
        auto* current = next;

        while (current != this) {
            auto* next = current->next;
            delete current;
            current = next;
        }
    }

    auto stream::link(stream& other) noexcept -> void {
        other.next = this;
        other.prev = prev;

        prev->next = &other;
        prev = &other;
    }

    auto stream::process_path(std::string_view path) -> void {
        auto storage = std::unique_ptr<char[]>(new char[path.size() + 2]);

        auto* const buffer = storage.get();
        std::size_t pos = 0;

        auto query = std::string_view();
        const auto query_start = path.find('?');
        if (query_start != std::string_view::npos) {
            query = path.substr(query_start + 1);
            path = path.substr(0, query_start);
        }

        request.path = percent_decode(path, buffer, pos);

        if (!query.empty()) process_query(query, buffer, pos);

        path_storage = std::move(storage);
    }

    auto stream::process_query(
        std::string_view query,
        char* buffer,
        std::size_t pos
    ) -> void {
        for (const auto entry : ext::string_range(query, "&")) {
            if (entry.empty()) continue;

            const auto delim = entry.find('=');

            if (delim == std::string_view::npos) {
                request.query.emplace(
                    percent_decode(entry, buffer, pos),
                    std::string_view()
                );
                continue;
            }

            auto key = entry.substr(0, delim);
            auto value = entry.substr(delim + 1);

            key = percent_decode(key, buffer, pos);
            value = percent_decode(value, buffer, pos);

            request.query.emplace(key, value);
        }
    }

    auto stream::recv_header(
        std::string_view name,
        std::string_view value
    ) -> void {
        TIMBER_TRACE(
            "Stream ID {} received header '{}: {}'",
            id,
            name,
            value
        );

        if (name == header::method) request.method = value;
        else if (name == header::path) process_path(value);
        else if (name == header::scheme) request.scheme = value;
        else if (name == header::authority) request.authority = value;
        else request.headers.emplace(name, value);
    }

    auto stream::unlink() noexcept -> void {
        next->prev = prev;
        prev->next = next;

        next = this;
        prev = this;
    }
}
