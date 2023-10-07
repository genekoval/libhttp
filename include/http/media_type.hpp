#pragma once

#include <fmt/format.h>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace http {
    class media_type {
        using size_type = std::uint16_t;

        enum { max_size = std::numeric_limits<size_type>::max() };

        std::string storage;

        size_type type_len = 0;
        size_type subtype_len = 0;
        size_type param_name_len = 0;
        size_type param_value_len = 0;
    public:
        struct name_value {
            std::string_view name;
            std::string_view value;

            constexpr name_value(
                std::string_view name,
                std::string_view value
            ) :
                name(name),
                value(value) {}

            constexpr auto operator==(const name_value& other) const noexcept
                -> bool = default;
        };

        constexpr media_type() = default;

        media_type(const char* str);

        media_type(std::string_view type);

        auto operator==(const media_type& other) const noexcept -> bool;

        auto parameter() const noexcept -> std::optional<name_value>;

        auto str() const noexcept -> std::string_view;

        auto subtype() const noexcept -> std::string_view;

        auto type() const noexcept -> std::string_view;
    };

    class invalid_media_type : public std::invalid_argument {
        std::string type;
    public:
        invalid_media_type(std::string_view type);

        auto invalid_type() const noexcept -> std::string_view;
    };

    namespace media {
        auto json() noexcept -> const media_type&;
        auto octet_stream() noexcept -> const media_type&;
        auto plain_text() noexcept -> const media_type&;
        auto utf8_text() noexcept -> const media_type&;
    }
}

namespace fmt {
    template <>
    struct formatter<http::media_type> : formatter<std::string_view> {
        template <typename FormatContext>
        constexpr auto format(const http::media_type& media, FormatContext& ctx)
            const {
            return formatter<std::string_view>::format(media.str(), ctx);
        }
    };
}
