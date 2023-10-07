#pragma once

#include "media_type.hpp"
#include "stream.hpp"

#include <curl/curl.h>
#include <ext/coroutine>
#include <optional>
#include <span>
#include <string_view>

namespace http {
    struct status {
        const long code = 0;

        status() = default;

        status(long code);

        operator long() const noexcept;

        auto ok() const noexcept -> bool;
    };

    class response {
        CURL* handle;
        std::string body;
    public:
        response(CURL* handle, std::string&& body);

        auto content_length() const noexcept -> long;

        auto content_type() const -> std::optional<media_type>;

        auto data() const& noexcept -> std::string_view;

        auto data() && noexcept -> std::string;

        auto header(std::string_view name) const
            -> std::optional<std::string_view>;

        auto ok() const noexcept -> bool;

        auto status() const noexcept -> http::status;
    };
}
