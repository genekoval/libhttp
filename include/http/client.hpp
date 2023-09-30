#pragma once

#include "session.hpp"
#include "json.hpp"
#include "request.h"

namespace http {
    class client final {
        const url base_url;
        http::session* const session;
    public:
        class request {
            http::request req;
            http::session* const session;

            auto read_error_file(
                const std::filesystem::path& path
            ) -> std::string;
        public:
            request(
                std::string_view method,
                const url& base_url,
                http::session* session
            );

            auto data() -> std::string;

            auto data(
                std::string&& data,
                std::optional<std::reference_wrapper<
                    const media_type
                >> content_type
            ) -> request&;

            auto data_view(
                std::string_view data,
                std::optional<std::reference_wrapper<
                    const media_type
                >> content_type
            ) -> request&;

            auto data_task() -> ext::task<std::string>;

            auto download(const std::filesystem::path& location) -> void;

            auto download_task(
                const std::filesystem::path& location
            ) -> ext::task<>;

            template <typename T>
            auto header(std::string_view name, const T& value) -> request& {
                req.header(name, value);
                return *this;
            }

            template <typename R>
            auto json() -> R {
                const auto res = req.perform();

                if (res.ok()) return json::parse(res.data());
                throw error_code(res.status(), res.data());
            }

            template <typename R>
            auto json_task() -> ext::task<R> {
                const auto res = co_await req.perform(*session);

                if (res.ok()) co_return json::parse(res.data());
                throw error_code(res.status(), res.data());
            }

            template <typename... Components>
            auto path(Components&&... components) -> request& {
                req.url.path_components(
                    std::forward<Components>(components)...
                );

                return *this;
            }

            auto pipe(FILE* file) -> void;

            auto pipe_task(FILE* file) -> ext::task<>;

            template <typename Query>
            auto query(std::string_view name, Query&& value) -> request& {
                req.url.query(name, value);
                return *this;
            }

            auto send() -> void;

            auto send_task() -> ext::task<>;

            auto text(
                std::string&& data,
                const media_type& content_type = media::utf8_text()
            ) -> request&;

            auto text_view(
                std::string_view data,
                const media_type& content_type = media::utf8_text()
            ) -> request&;

            template <typename R>
            auto try_json() -> std::optional<R> {
                const auto res = req.perform();

                if (res.status() == 404) return std::nullopt;
                if (res.ok()) return json::parse(res.data());
                throw error_code(res.status(), res.data());
            }

            template <typename R>
            auto try_json_task() -> ext::task<std::optional<R>> {
                const auto res = co_await req.perform(*session);

                if (res.status() == 404) co_return std::nullopt;
                if (res.ok()) co_return json::parse(res.data());
                throw error_code(res.status(), res.data());
            }

            auto upload(
                const std::filesystem::path& path,
                const media_type& content_type = media::octet_stream()
            ) -> request&;
        };

        client(std::string_view base_url);

        client(std::string_view base_url, http::session& session);

        auto del() const -> request;

        template <typename... Path>
        requires (sizeof...(Path) > 0)
        auto del(Path&&... path) const -> request {
            auto req = del();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto get() const -> request;

        template <typename... Path>
        requires (sizeof...(Path) > 0)
        auto get(Path&&... path) const -> request {
            auto req = get();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto head() const -> request;

        template <typename... Path>
        requires (sizeof...(Path) > 0)
        auto head(Path&&... path) const -> request {
            auto req = head();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto method(std::string_view method) const -> request;

        auto post() const -> request;

        template <typename... Path>
        requires (sizeof...(Path) > 0)
        auto post(Path&&... path) const -> request {
            auto req = post();
            req.path(std::forward<Path>(path)...);
            return req;
        }

        auto put() const -> request;

        template <typename... Path>
        requires (sizeof...(Path) > 0)
        auto put(Path&&... path) const -> request {
            auto req = put();
            req.path(std::forward<Path>(path)...);
            return req;
        }
    };
}
