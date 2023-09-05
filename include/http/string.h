#pragma once

#include <string_view>

namespace http {
    class string {
        char* str;
    public:
        string();

        string(char* data);

        string(const string&) = delete;

        string(string&& other);

        ~string();

        operator std::string_view() const noexcept;

        auto operator=(const string&) -> string& = delete;

        auto operator=(string&& other) -> string&;

        auto data() const noexcept -> const char*;
    };
}
