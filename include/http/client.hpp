#pragma once

#include "json.hpp"
#include "request.h"
#include "session.hpp"

namespace http {
    class client final {
        const url base_url;
        http::session* const session;
    public:
        class request {
            http::request req;
            http::session* const session;

            template <typename T>
            auto decode(response&& res) -> T {
                res.check_status();

                const auto content_type = res.content_type();
                if (!content_type) throw error("Missing 'Content-Type' header");

                if constexpr (std::convertible_to<http::json, T>) {
                    if (*content_type == media::json()) {
                        return json::parse(std::move(res).data());
                    }
                }

                throw error("Unsupported content type '{}'", *content_type);
            }

            auto read_error_file(const std::filesystem::path& path)
                -> std::string;

            auto text(response&& res) -> std::string;
        public:
            request(
                std::string_view method,
                const url& base_url,
                http::session* session
            );

            auto data() -> std::string;

            auto data(
                std::string&& data,
                std::optional<std::reference_wrapper<const media_type>>
                    content_type
            ) -> request&;

            auto data_async() -> ext::task<std::string>;

            auto data_view(
                std::string_view data,
                std::optional<std::reference_wrapper<const media_type>>
                    content_type
            ) -> request&;

            auto download(const std::filesystem::path& location) -> void;

            auto download_task(const std::filesystem::path& location)
                -> ext::task<>;

            template <typename T>
            auto header(std::string_view name, const T& value) -> request& {
                req.header(name, value);
                return *this;
            }

            template <typename... Components>
            auto path(Components&&... components) -> request& {
                req.url.path_components(std::forward<Components>(components)...
                );

                return *this;
            }

            auto pipe(FILE* file) -> void;

            auto pipe_async(FILE* file) -> ext::task<>;

            template <typename Query>
            auto query(std::string_view name, Query&& value) -> request& {
                req.url.query(name, value);
                return *this;
            }

            template <typename T = void>
            auto send() -> T {
                return decode<T>(req.perform());
            }

            template <typename T = void>
            auto send_async() -> ext::task<T> {
                co_return decode<T>(co_await req.perform(*session));
            }

            template <>
            auto send<void>() -> void;

            template <>
            auto send_async<void>() -> ext::task<>;

            template <>
            auto send<std::string>() -> std::string;

            template <>
            auto send_async<std::string>() -> ext::task<std::string>;

            auto text(
                std::string&& data,
                const media_type& content_type = media::utf8_text()
            ) -> request&;

            auto text_view(
                std::string_view data,
                const media_type& content_type = media::utf8_text()
            ) -> request&;

            auto upload(
                const std::filesystem::path& path,
                const media_type& content_type = media::octet_stream()
            ) -> request&;
        };

        client(std::string_view base_url);

        client(std::string_view base_url, http::session& session);

        auto del() const -> request;

        template <typename... Path>
        requires(sizeof...(Path) > 0)
        auto del(Path&&... path) const -> request {
            auto req = del();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto get() const -> request;

        template <typename... Path>
        requires(sizeof...(Path) > 0)
        auto get(Path&&... path) const -> request {
            auto req = get();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto head() const -> request;

        template <typename... Path>
        requires(sizeof...(Path) > 0)
        auto head(Path&&... path) const -> request {
            auto req = head();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto method(std::string_view method) const -> request;

        auto post() const -> request;

        template <typename... Path>
        requires(sizeof...(Path) > 0)
        auto post(Path&&... path) const -> request {
            auto req = post();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto put() const -> request;

        template <typename... Path>
        requires(sizeof...(Path) > 0)
        auto put(Path&&... path) const -> request {
            auto req = put();
            req.path(std::forward<Path>(path)...);
            return req;
        }
    };
}
