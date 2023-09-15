#pragma once

#include "../response.hpp"

namespace http::server {
    template <>
    struct response_type<std::string_view> {
        static auto send(response& res, std::string_view string) -> void {
            res.content_type(media::utf8_text());
            res.content_length(string.size());
            res.data = std::string(string);
        }
    };

    template <>
    struct response_type<std::string> {
        static auto send(response& res, std::string&& string) -> void {
            res.content_type(media::utf8_text());
            res.content_length(string.size());
            res.data = std::forward<std::string>(string);
        }
    };

    template <>
    struct response_type<const char*> {
        static auto send(response& res, const char* str) -> void {
            response_type<std::string_view>::send(res, str);
        }
    };
}
