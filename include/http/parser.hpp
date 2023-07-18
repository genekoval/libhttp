#pragma once

#include "error.h"

#include <chrono>
#include <concepts>
#include <filesystem>
#include <optional>
#include <uuid++/uuid++>

namespace http {
    struct parser_error : std::runtime_error {
        parser_error(const std::string& what) : runtime_error(what) {}
    };

    template <typename T>
    struct parser {};

    template <>
    struct parser<std::string_view> {
        static auto parse(std::string_view string) -> std::string_view {
            return string;
        }
    };

    template <>
    struct parser<std::string> {
        static auto parse(std::string_view string) -> std::string {
            return std::string(string);
        }
    };

    template <typename T>
    struct parser<std::optional<T>> {
        static auto parse(std::string_view string) -> std::optional<T> {
            return parser<T>::parse(string);
        }
    };

    template <>
    struct parser<bool> {
        static auto parse(std::string_view string) -> bool {
            if (string.size() == 1) {
                switch (string[0]) {
                    case 't':
                    case 'y':
                        return true;
                    case 'f':
                    case 'n':
                        return false;
                    default:
                        break;
                }
            }
            else if (string == "true" || string == "yes") return true;
            else if (string == "false" || string == "no") return false;

            throw parser_error("Expect (t)rue/(f)alse or (y)es/(n)o");
        }
    };

    template <std::integral T>
    class parser<T> {
        static constexpr auto min = std::numeric_limits<T>::min();
        static constexpr auto max = std::numeric_limits<T>::max();
        static constexpr auto base = 0;

        [[noreturn]]
        static auto out_of_range(std::string_view argument) {
            throw parser_error(fmt::format(
                "Argument '{}' is outside the range of {} and {}",
                argument,
                min,
                max
            ));
        }
    public:
        static auto parse(std::string_view argument) -> T {
            const auto string = parser<std::string>::parse(argument);

            try {
                if constexpr (std::is_same_v<long long, T>) {
                    return std::stoll(string);
                }
                else if constexpr (std::is_same_v<unsigned long long, T>) {
                    return std::stoull(string);
                }
                else if constexpr (std::is_same_v<long, T>) {
                    return std::stol(string);
                }
                else if constexpr (std::is_same_v<unsigned long, T>) {
                    return std::stoul(string);
                }
                else if constexpr (std::is_signed_v<T>) {
                    const auto value = std::stoi(string);
                    if (value > max || value < min) out_of_range(argument);
                    return static_cast<T>(value);
                }
                else {
                    const auto value = std::stoul(string);
                    if (value > max || value < min) out_of_range(argument);
                    return static_cast<T>(value);
                }
            }
            catch (const std::invalid_argument& ex) {
                throw parser_error("Expect an integer");
            }
            catch (const std::out_of_range& ex) {
                out_of_range(argument);
            }

            __builtin_unreachable();
        }
    };

    template <typename Rep, typename Period>
    struct parser<std::chrono::duration<Rep, Period>> {
        static auto parse(
            std::string_view string
        ) -> std::chrono::duration<Rep, Period> {
            const auto value = parser<Rep>::parse(string);
            return std::chrono::duration<Rep, Period>(value);
        }
    };

    template <>
    struct parser<std::filesystem::path> {
        static auto parse(std::string_view string) -> std::filesystem::path {
            return string;
        }
    };

    template <>
    struct parser<UUID::uuid> {
        static auto parse(std::string_view string) -> UUID::uuid {
            return UUID::uuid(string);
        }
    };
}
