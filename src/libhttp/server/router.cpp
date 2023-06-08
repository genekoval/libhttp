#include <http/server/response/string.hpp>
#include <http/server/router.hpp>

#include <timber/timber>

namespace http::server {
    router::router(path&& paths) : paths(std::forward<path>(paths)) {}

    auto router::route(stream& stream) -> ext::task<bool> {
        auto match = paths.find(stream.request.path);
        if (!match) {
            stream.response.status = 404;
            co_return true;
        }

        stream.request.params = std::move(match->params);
        auto& methods = *match->value;

        auto* handler = methods.find(stream.request.method);
        if (!handler) {
            stream.response.status = 405;
            stream.response.headers.emplace("allow", methods.allowed());
            co_return true;
        }

        try {
            co_await handler->handle(stream);
        }
        catch (const stream_aborted&) {
            TIMBER_DEBUG("Stream ID {} aborted", stream.id);
            co_return false;
        }
        catch (const error_code& error) {
            TIMBER_DEBUG(
                "Stream ID {}, Status: {} ({})",
                stream.id,
                error.code(),
                error.what()
            );

            stream.response.status = error.code();
            stream.response.send(error.what());
        }
        catch (const std::exception& ex) {
            TIMBER_ERROR(
                "Stream ID {}, Status: 500 ({})",
                stream.id,
                ex.what()
            );

            stream.response.status = 500;
        }

        co_return stream.open;
    }
}
