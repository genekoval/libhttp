#pragma once

#include "error.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace http {
    struct name_value {
        std::string name;
        std::string value;

        constexpr name_value(std::string_view name, std::string_view value) :
            name(name),
            value(value)
        {}

        constexpr auto operator==(
            const name_value& other
        ) const noexcept -> bool = default;
    };

    struct media_type {
        std::string type;
        std::string subtype;
        std::optional<name_value> parameter;

        constexpr media_type(std::string_view type) {
            auto begin = type.begin();
            auto it = begin;
            const auto end = type.end();

            while (it != end && *it != '/') ++it;
            if (it == end) throw invalid_media_type(type);

            this->type = {begin, it};
            begin = ++it;

            while (it != end && *it != ';') ++it;
            if (it == begin) throw invalid_media_type(type);

            subtype = {begin, it};

            if (*it == ';') {
                if (*++it == ' ') ++it;

                begin = it;

                while (it != end && *it != '=') ++it;
                if (it == end) throw invalid_media_type(type);

                auto name = std::string_view(begin, it);
                begin = ++it;

                while (it != end) ++it;
                if (it == begin) throw invalid_media_type(type);

                auto value = std::string_view(begin, it);

                parameter.emplace(name, value);
            }
        }

        constexpr auto operator==(
            const media_type& other
        ) const noexcept -> bool = default;
    };

    namespace media {
        constexpr auto json = media_type("application/json");
        constexpr auto plain_text = media_type("text/plain");
        constexpr auto utf8_text = media_type("text/plain; charset=UTF-8");
    }
}

namespace fmt {
    template <>
    struct formatter<http::media_type> {
        template <typename ParseContext>
        constexpr auto parse(ParseContext& ctx) const {
            return ctx.begin();
        }

        template <typename FormatContext>
        constexpr auto format(
            const http::media_type& media,
            FormatContext& ctx
        ) const {
            auto it = format_to(ctx.out(), "{}/{}", media.type, media.subtype);

            if (media.parameter) {
                const auto& param = *media.parameter;
                it = format_to(ctx.out(), "; {}={}", param.name, param.value);
            }

            return it;
        }
    };
}
