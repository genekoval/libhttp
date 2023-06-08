#pragma once

#include <http/server/request.hpp>

namespace http::server::extractor {
    struct method {
        std::string_view value;

        method(request& request) : value(request.method) {}
    };
}
