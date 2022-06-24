#pragma once

#include <string_view>

namespace http {
    class string {
        char* data;
    public:
        string();

        string(char* data);

        ~string();

        operator std::string_view() const noexcept;
    };
}
