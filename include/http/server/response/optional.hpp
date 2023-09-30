#pragma once

#include "string.hpp"

namespace http::server {
    template <typename T>
    struct response_type<std::optional<T>> {
        static auto send(response& res, std::optional<T>&& opt) -> void {
            if (opt) {
                response_type<T>::send(res, *std::move(opt));
                return;
            }

            res.status = 404;
            response_type<std::string_view>::send(res, "Not Found");
        }
    };
}
