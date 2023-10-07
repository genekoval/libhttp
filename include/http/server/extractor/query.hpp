#pragma once

#include "fixed_string.hpp"
#include "mapped_type.hpp"

#include <http/server/request.hpp>

namespace http::server::extractor {
    template <fixed_string Name, typename T = std::optional<std::string_view>>
    struct query : detail::mapped_type<T> {
        query(request& request) :
            detail::mapped_type<T>(
                Name.str(),
                request.query_param<T>(Name.str())
            ) {}
    };
}
