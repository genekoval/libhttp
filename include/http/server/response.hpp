#pragma once

#include <netcore/netcore>
#include <string>
#include <unordered_map>
#include <vector>

namespace http::server {
    struct file {
        netcore::fd fd;
        std::size_t size;
        std::string content_type;
    };

    struct response;

    template <typename T>
    struct response_type {};

    template <typename T>
    concept response_data = requires(response& res, T&& t) {
        { response_type<std::remove_cvref_t<T>>::send(
            res,
            std::forward<T>(t)
        ) } -> std::same_as<void>;
    };

    struct response {
        int status = 200;
        std::unordered_map<std::string, std::string> headers;
        std::variant<std::monostate, std::string, file> data;
        std::size_t written = 0;

        auto content_length(std::size_t length) -> void {
            headers.emplace("content-length", std::to_string(length));
        }

        auto content_type(std::string_view type) -> void {
            headers.emplace("content-type", type);
        }

        template <response_data T>
        auto send(T&& t) -> void {
            response_type<T>::send(*this, std::forward<T>(t));
        }
    };
}
