#pragma once

#include "../response.hpp"

namespace http::server {
    template <>
    struct response_type<file> {
        static auto send(response& res, file&& file) -> void {
            res.content_type(media_type(file.content_type));
            res.content_length(file.size);
            res.data = std::move(file);
        }
    };
}
