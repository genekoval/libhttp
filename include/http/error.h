#pragma once

#include <fmt/core.h>

namespace http {
    class client_error : public std::runtime_error {
    public:
        template <typename ...Args>
        client_error(std::string_view format_str, Args&&... args) :
            std::runtime_error(fmt::format(
                fmt::runtime(format_str),
                std::forward<Args>(args)...
            ))
        {}
    };

    class error : public std::runtime_error {
        static auto format_message(
            fmt::string_view format,
            fmt::format_args args
        ) -> std::string {
            return fmt::vformat(format, args);
        }
    public:
        error(std::string_view what) : runtime_error(what.data()) {}

        template <typename... T>
        error(fmt::format_string<T...> format, T&&... args) :
            runtime_error(format_message(
                format,
                fmt::make_format_args(args...)
            ))
        {}
    };

    class error_code : public error {
        int errorc;
    public:
        error_code(int code, std::string_view what) :
            error(what),
            errorc(code)
        {}

        template <typename... T>
        error_code(int code, fmt::format_string<T...> format, T&&... args) :
            error(format, std::forward<T>(args)...),
            errorc(code)
        {}

        auto code() const noexcept -> int {
            return errorc;
        }
    };

    struct stream_aborted : std::exception {
        auto what() const noexcept -> const char* override {
            return "Stream aborted";
        }
    };
}
