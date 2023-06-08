#pragma once

#include <string_view>

namespace http::server::extractor::detail {
    template <typename T>
    struct mapped_type {
        const std::string_view name;
        const T value;

        mapped_type(std::string_view name, T&& value) :
            name(name),
            value(std::forward<T>(value))
        {}
    };
}
