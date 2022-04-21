#pragma once

#include <fmt/core.h>

namespace http {
    class client_error : public std::runtime_error {
    public:
        template <typename ...Args>
        client_error(std::string_view format_str, Args&&... args) :
            std::runtime_error(fmt::format(
                format_str,
                std::forward<Args>(args)...
            ))
        {}
    };
}
