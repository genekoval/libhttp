#pragma once

#include "fixed_string.hpp"
#include "mapped_type.hpp"

#include <http/server/request.hpp>

namespace http::server::extractor {
    template <fixed_string Name, typename T = std::string_view>
    struct path : detail::mapped_type<T> {
        path(request& request) :
            detail::mapped_type<T>(
                Name.str(),
                request.path_param<T>(Name.str())
            ) {}
    };
}
