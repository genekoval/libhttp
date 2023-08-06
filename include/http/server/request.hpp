#pragma once

#include <http/parser.hpp>

#include <ext/coroutine>
#include <optional>
#include <span>
#include <unordered_map>

namespace http::server {
    namespace detail {
        template <typename T>
        struct is_optional : std::false_type {};

        template <typename T>
        struct is_optional<std::optional<T>> : std::true_type {};

        template <typename T>
        inline constexpr bool is_optional_v = is_optional<T>::value;

        template <typename T, typename Map>
        auto parse(
            const Map& map,
            std::string_view name,
            std::string_view description
        ) -> T {
            using key_type = typename Map::key_type;

            auto key = key_type();

            if constexpr (
                std::same_as<key_type, std::string_view>
            ) key = name;
            else key = std::string(name);

            const auto result = map.find(key);

            if (result != map.end()) {
                const auto value = result->second;

                if constexpr (is_optional_v<T>) {
                    if (value.empty()) return T();
                }

                try {
                    return parser<T>::parse(value);
                }
                catch (const std::exception& ex) {
                    throw error_code(
                        400,
                        "Failed to parse {} '{}': {}",
                        description,
                        name,
                        ex.what()
                    );
                }
            }
            else if constexpr (!is_optional_v<T>) {
                throw error_code(
                    400,
                    "Missing required {} '{}'",
                    description,
                    name
                );
            }

            return T();
        }
    }

    struct request {
        std::string method;
        std::string_view path;
        std::unordered_map<std::string_view, std::string_view> params;
        std::unordered_map<std::string_view, std::string_view> query;
        std::unordered_map<std::string, std::string> headers;
        std::string scheme;
        std::string authority;
        std::span<const std::byte> data;
        bool eof = false;
        bool discard = false;
        ext::continuation<> continuation;

        template <typename T>
        auto header(std::string_view name) const -> T {
            return detail::parse<T>(headers, name, "header");
        }

        template <typename T>
        auto path_param(std::string_view name) const -> T {
            return detail::parse<T>(params, name, "path parameter");
        }

        template <typename T>
        auto query_param(std::string_view name) const -> T {
            return detail::parse<T>(query, name, "query parameter");
        }
    };
}
