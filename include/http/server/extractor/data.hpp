#pragma once

#include <http/json.hpp>
#include <http/server/request.hpp>

namespace http::server::extractor {
    template <typename T>
    struct data {};

    template <typename T>
    concept readable = requires(request& request) {
        { data<T>::read(request) } -> std::same_as<ext::task<T>>;
    };

    template <>
    struct data<std::string> {
        static auto read(request& request) -> ext::task<std::string>;
    };

    template <>
    struct data<json> {
        static auto read(request& request) -> ext::task<json>;
    };
}
