#include <http/server/extractor/data.hpp>

namespace http::server::extractor {
    auto data<std::string>::read(request& request) -> ext::task<std::string> {
        const auto length =
            request.header<std::optional<std::size_t>>("content-length");

        auto string = std::string();
        if (length) string.reserve(*length);

        while (!request.eof) {
            co_await request.continuation;

            if (!request.data.empty()) {
                string.append(
                    reinterpret_cast<const char*>(request.data.data()),
                    request.data.size()
                );
            }
        };

        co_return string;
    }

    auto data<json>::read(request& request) -> ext::task<json> {
        constexpr auto expected = "application/json";

        const auto type = request.header<std::string_view>("content-type");

        if (type != expected) {
            throw error_code(400, "Expect content of type '{}'", expected);
        }

        co_return json::parse(co_await data<std::string>::read(request));
    }
}