#include <http/server/request.hpp>

namespace http::server {
    auto request::content_type() const -> media_type {
        return header<std::string_view>("content-type");
    }

    auto request::expect_content_type(
        const media_type& expected
    ) const -> void {
        if (content_type() != expected) {
            throw error_code(400, "Expect content of type '{}'", expected);
        }
    }
}
