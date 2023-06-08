#pragma once

#include "../response.hpp"

#include <http/json.hpp>

namespace http::server {
    template <>
    struct response_type<json> {
        static auto send(response& res, const json& json) -> void {
            auto string = json.dump();

            res.content_type("application/json");
            res.content_length(string.size());
            res.data = std::move(string);
        }
    };
}
