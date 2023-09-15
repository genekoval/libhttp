#pragma once

#include "../response.hpp"

#include <http/json.hpp>

namespace http::server {
    template <>
    struct response_type<json> {
        static auto send(response& res, const json& json) -> void {
            auto string = json.dump();

            res.content_type(media::json());
            res.content_length(string.size());
            res.data = std::move(string);
        }
    };

    template <std::convertible_to<json> T>
    struct response_type<T> {
        static auto send(response& res, const T& t) -> void {
            response_type<json>::send(res, t);
        }
    };
}
