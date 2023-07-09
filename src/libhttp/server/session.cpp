#include <http/server/session.hpp>

#include <netcore/netcore>

using namespace std::literals;

namespace {
    auto make_nv(
        std::string_view name,
        std::string_view value,
        std::uint8_t flags = NGHTTP2_NV_FLAG_NONE
    ) -> nghttp2_nv {
        return {
            .name = (std::uint8_t*) name.data(),
            .value = (std::uint8_t*) value.data(),
            .namelen = name.size(),
            .valuelen = value.size(),
            .flags = flags
        };
    }

    auto data_source_read_callback(
        nghttp2_session* handle,
        std::int32_t stream_id,
        std::uint8_t* buf,
        std::size_t length,
        std::uint32_t* data_flags,
        nghttp2_data_source* source,
        void* user_data
    ) -> ssize_t {
        auto& res = *reinterpret_cast<http::server::response*>(source->ptr);

        return std::visit([&] <typename T> (const T& t) -> ssize_t {
            ssize_t written = 0;

            if constexpr (std::same_as<T, std::string>) {
                const auto max = std::min(
                    t.size() - res.written,
                    length
                );

                written = t.copy(
                    reinterpret_cast<char*>(buf),
                    max,
                    res.written
                );

                res.written += written;

                if (res.written == t.size()) {
                    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
                }
            }
            else if constexpr (std::same_as<T, http::server::file>) {
                const auto max = std::min(
                    t.size - res.written,
                    length
                );

                const auto ret = read(t.fd, buf, max);
                if (ret == -1) {
                    TIMBER_ERROR(
                        "Reading from fd ({}) failed: {}",
                        t.fd,
                        strerror(errno)
                    );
                    return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
                }

                written = ret;
                res.written += ret;

                TIMBER_TRACE(
                    "fd ({}) read {:L} byte{} ({:L} remaining)",
                    t.fd,
                    ret,
                    ret == 1 ? "" : "s",
                    t.size - res.written
                );

                if (res.written == t.size) {
                    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
                }
            }

            return written;
        }, res.data);
    }

    auto on_begin_headers_callback(
        nghttp2_session* handle,
        const nghttp2_frame* frame,
        void* user_data
    ) -> int {
        if (
            frame->hd.type != NGHTTP2_HEADERS ||
            frame->headers.cat != NGHTTP2_HCAT_REQUEST
        ) return 0;

        auto& session = *reinterpret_cast<http::server::session*>(user_data);
        auto& stream = session.make_stream(frame->hd.stream_id);

        nghttp2_session_set_stream_user_data(
            handle,
            frame->hd.stream_id,
            &stream
        );

        return 0;
    }

    auto on_data_chunk_recv_callback(
        nghttp2_session* handle,
        std::uint8_t flags,
        std::int32_t stream_id,
        const std::uint8_t* data,
        size_t len,
        void *user_data
    ) -> int {
        auto& stream = *reinterpret_cast<http::server::stream*>(
            nghttp2_session_get_stream_user_data(handle, stream_id)
        );

        if (stream.request.discard) {
            TIMBER_DEBUG(
                "Stream ID {} discarded data chunk of {:L} bytes",
                stream_id,
                len
            );

            return 0;
        }

        if (stream.request.continuation) {
            TIMBER_TRACE(
                "Stream ID {} received data chunk of {:L} bytes",
                stream_id,
                len
            );

            stream.request.data = std::span<const std::byte>(
                reinterpret_cast<const std::byte*>(data),
                len
            );

            stream.request.continuation.resume();

            return 0;
        }

        TIMBER_DEBUG("Stream ID {} attempting to pause data recv", stream_id);

        auto& session = *reinterpret_cast<http::server::session*>(user_data);
        session.pause = &stream.request.continuation;

        return NGHTTP2_ERR_PAUSE;
    }

    auto on_header_callback(
        nghttp2_session* handle,
        const nghttp2_frame* frame,
        const std::uint8_t* name,
        std::size_t namelen,
        const std::uint8_t* value,
        std::size_t valuelen,
        std::uint8_t flags,
        void* user_data
    ) -> int {
        switch (frame->hd.type) {
            case NGHTTP2_HEADERS: {
                if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) break;

                if (auto* stream = reinterpret_cast<http::server::stream*>(
                    nghttp2_session_get_stream_user_data(
                        handle,
                        frame->hd.stream_id
                    )
                )) stream->recv_header(
                    std::string_view(
                        reinterpret_cast<const char*>(name),
                        namelen
                    ),
                    std::string_view(
                        reinterpret_cast<const char*>(value),
                        valuelen
                    )
                );
                break;
            }
        }

        return 0;
    }

    auto on_frame_recv_callback(
        nghttp2_session* handle,
        const nghttp2_frame* frame,
        void* user_data
    ) -> int {
        auto* const stream_ptr =
            nghttp2_session_get_stream_user_data(handle, frame->hd.stream_id);

        if (!stream_ptr) return 0;

        auto& stream = *reinterpret_cast<http::server::stream*>(stream_ptr);
        auto& session = *reinterpret_cast<http::server::session*>(user_data);

        switch (frame->hd.type) {
            case NGHTTP2_HEADERS:
                TIMBER_TRACE("Stream ID {} header frame complete", stream.id);
                session.handle_request(stream);
                break;
            case NGHTTP2_DATA:
                TIMBER_TRACE("Stream ID {} data frame complete", stream.id);
                if (
                    frame->hd.flags & NGHTTP2_FLAG_END_STREAM &&
                    stream.request.continuation
                ) {
                    stream.request.data = std::span<const std::byte>();
                    stream.request.eof = true;
                    stream.request.continuation.resume();
                }
                break;
        }

        return 0;
    }

    auto on_invalid_header_callback(
        nghttp2_session* session,
        const nghttp2_frame* frame,
        const std::uint8_t* name,
        std::size_t namelen,
        const std::uint8_t* value,
        std::size_t valuelen,
        std::uint8_t flags,
        void* user_data
    ) -> int {
        TIMBER_DEBUG(
            "Received invalid header ({}: {})",
            std::string_view(
                reinterpret_cast<const char*>(name),
                namelen
            ),
            std::string_view(
                reinterpret_cast<const char*>(value),
                valuelen
            )
        );

        return 0;
    }

    auto on_stream_close(
        nghttp2_session* handle,
        std::int32_t stream_id,
        std::uint32_t error_code,
        void* user_data
    ) -> int {
        TIMBER_FUNC();

        if (auto* stream = reinterpret_cast<http::server::stream*>(
            nghttp2_session_get_stream_user_data(handle, stream_id)
        )) {
            if (stream->active) stream->open = false;
            else delete stream;
        }

        return 0;
    }
}

namespace http::server {
    session::session(
        netcore::ssl::socket&& socket,
        std::size_t buffer_size,
        server::router& router
    ) :
        socket(std::forward<netcore::ssl::socket>(socket), buffer_size),
        router(router)
    {
        nghttp2_session_callbacks* callbacks = nullptr;
        nghttp2_session_callbacks_new(&callbacks);

        nghttp2_session_callbacks_set_on_begin_headers_callback(
            callbacks,
            on_begin_headers_callback
        );

        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
            callbacks,
            on_data_chunk_recv_callback
        );

        nghttp2_session_callbacks_set_on_frame_recv_callback(
            callbacks,
            on_frame_recv_callback
        );

        nghttp2_session_callbacks_set_on_header_callback(
            callbacks,
            on_header_callback
        );

        nghttp2_session_callbacks_set_on_invalid_header_callback(
            callbacks,
            on_invalid_header_callback
        );

        nghttp2_session_callbacks_set_on_stream_close_callback(
            callbacks,
            on_stream_close
        );

        nghttp2_session_server_new(&handle, callbacks, this);
        nghttp2_session_callbacks_del(callbacks);
    }

    session::~session() {
        streams.delete_all();
        nghttp2_session_del(handle);
    }

    auto session::handle_connection() -> ext::task<> {
        co_await send_server_connection_header();
        co_await recv();
        socket.shutdown();
        co_await tasks.await();
    }

    auto session::handle_request(stream& stream) -> ext::detached_task {
        const auto counter = tasks.increment();
        stream.active = true;

        TIMBER_DEBUG("{}", stream);

        try {
            if (co_await router.route(stream)) co_await respond(stream);
        }
        catch (const std::exception& ex) {
            TIMBER_ERROR("Session handler failed: {}", ex.what());
        }
        catch (...) {
            TIMBER_ERROR("Session handler failed");
        }

        stream.active = false;
        if (!stream.open) delete &stream;
    }

    auto session::make_stream(std::int32_t id) -> stream& {
        auto* stream = new server::stream(id);

        streams.link(*stream);

        return *stream;
    }

    auto session::recv() -> ext::task<> {
        auto bytes = std::span<const std::byte>();

        do {
            if (pause) pause = nullptr;
            else bytes = co_await socket.read();

            const auto rv = nghttp2_session_mem_recv(
                handle,
                reinterpret_cast<const std::uint8_t*>(bytes.data()),
                bytes.size()
            );

            if (rv < 0) throw std::runtime_error(nghttp2_strerror(rv));
            else if (rv == 0 && pause) co_await *pause;
            // Always try calling nghttp2_session_mem_send() after calling
            // nghttp2_session_mem_recv() to ensure that we don't miss
            // sending any pending WINDOW_UPDATE frames.
            else co_await send();
        } while (!bytes.empty());
    }

    auto session::respond(stream& stream) -> ext::task<> {
        if (pause && pause->awaiting()) {
            stream.request.discard = true;
            co_await *pause;
        }

        auto& res = stream.response;

        auto headers = std::vector<nghttp2_nv>();
        headers.push_back(make_nv(":status", fmt::to_string(res.status)));

        for (const auto& [name, value] : res.headers) {
            headers.push_back(make_nv(name, value));
        }

        auto provider = nghttp2_data_provider {
            .source = {
                .ptr = &res,
            },
            .read_callback = data_source_read_callback
        };

        const auto rv = nghttp2_submit_response(
            handle,
            stream.id,
            headers.data(),
            headers.size(),
            std::holds_alternative<std::monostate>(res.data) ?
                nullptr : &provider
        );

        if (rv != 0) throw std::runtime_error(nghttp2_strerror(rv));

        co_await send();
    }

    auto session::send() -> ext::task<> {
        const std::uint8_t* src = nullptr;

        while (const auto length = nghttp2_session_mem_send(handle, &src)) {
            if (length < 0) throw std::runtime_error(nghttp2_strerror(length));
            co_await socket.write(src, length);
        }

        co_await socket.flush();
    }

    auto session::send_server_connection_header() -> ext::task<> {
        auto settings = std::array {
            nghttp2_settings_entry {
                NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS,
                100
            }
        };

        const auto rv = nghttp2_submit_settings(
            handle,
            NGHTTP2_FLAG_NONE,
            settings.data(),
            settings.size()
        );

        if (rv != 0) throw std::runtime_error(nghttp2_strerror(rv));
        co_await send();
    }
}