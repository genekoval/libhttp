#pragma once

#include <string_view>

namespace http::server::extractor {
    template <std::size_t Size>
    struct fixed_string {
        char chars[Size] = {};

        constexpr fixed_string(const char (&chars)[Size]) {
            for (auto i = 0; i < Size; ++i) {
                this->chars[i] = chars[i];
            }
        }

        constexpr auto str() const noexcept -> std::string_view {
            return chars;
        }
    };
}
