#pragma once

#include <stdexcept>

namespace http::server {
    struct error : virtual std::runtime_error {
        error() : runtime_error("error") {}

        virtual ~error() = default;

        virtual auto http_code() const noexcept -> int = 0;
    };
}
