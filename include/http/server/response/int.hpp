#pragma once

#include "../response.hpp"

namespace http::server {
    template <>
    struct response_type<int> {
        static auto send(response& res, int status) -> void {
            res.status = status;
        }
    };
}
