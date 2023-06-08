#pragma once

#include "handler.hpp"

#define HTTP_METHOD(name, str) \
    template <typename F> \
    auto name(F&& f) -> method_router& { \
        return use(str, std::forward<F>(f)); \
    } \
\
    template <typename T, typename F> \
    auto name(T& t, F&& f) -> method_router& { \
        return use(str, t, std::forward<F>(f)); \
    }

#define HTTP_METHOD_FN(name) \
    template <typename F> \
    auto name(F&& f) -> method_router { \
        auto router = method_router(); \
        router.name(std::forward<F>(f)); \
        return router; \
    } \
\
    template <typename T, typename F> \
    auto name(T& t, F&& f) -> method_router { \
        auto router = method_router(); \
        router.name(t, std::forward<F>(f)); \
        return router; \
    }

namespace http::server {
    class method_router {
        std::string allowed_cache;
        bool cache_invalid = true;
        std::unordered_map<std::string_view, std::unique_ptr<handler>> methods;
    public:
        method_router() = default;

        method_router(const method_router&) = delete;

        method_router(method_router&&) = default;

        auto operator=(const method_router&) -> method_router& = delete;

        auto operator=(method_router&&) -> method_router& = default;

        auto allowed() -> std::string_view;

        auto allowed() const -> std::string;

        auto find(std::string_view method) -> handler*;

        template <typename F>
        auto use(std::string_view method, F&& f) -> method_router& {
            methods.emplace(method, make_handler(std::forward<F>(f)));
            cache_invalid = true;
            return *this;
        }

        template <typename T, typename F>
        auto use(std::string_view method, T& t, F&& f) -> method_router& {
            methods.emplace(method, make_handler(t, std::forward<F>(f)));
            cache_invalid = true;
            return *this;
        }


        HTTP_METHOD(del,  "DELETE")
        HTTP_METHOD(get,  "GET")
        HTTP_METHOD(head, "HEAD")
        HTTP_METHOD(post, "POST")
        HTTP_METHOD(put,  "PUT")
    };

    template <typename F>
    auto method(std::string_view method, F&& f) -> method_router {
        auto router = method_router();
        router.use(method, std::forward<F>(f));
        return router;
    }

    template <typename T, typename F>
    auto method(std::string_view method, T& t, F&& f) -> method_router {
        auto router = method_router();
        router.use(method, t, std::forward<F>(f));
        return router;
    }

    HTTP_METHOD_FN(del)
    HTTP_METHOD_FN(get)
    HTTP_METHOD_FN(head)
    HTTP_METHOD_FN(post)
    HTTP_METHOD_FN(put)
}

#undef HTTP_METHOD
#undef HTTP_METHOD_FN

template <>
struct fmt::formatter<http::server::method_router> :
    formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const http::server::method_router& router, FormatContext& ctx) {
        return formatter<std::string_view>::format(router.allowed(), ctx);
    }
};
