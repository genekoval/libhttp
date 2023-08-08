#pragma once

#include "extractor/extractor.hpp"
#include "stream.hpp"

namespace http::server {
    struct handler {
        virtual ~handler() = default;

        virtual auto handle(stream& stream) -> ext::task<> = 0;
    };

    namespace detail {
        template <std::constructible_from<request&> T>
        auto make_extractor(request& request) -> ext::task<T> {
            co_return T(request);
        }

        template <extractor::readable T>
        auto make_extractor(request& request) -> ext::task<T> {
            co_return co_await extractor::data<T>::read(request);
        }

        template <typename Args, std::size_t... I>
        auto extract(
            request& request,
            std::index_sequence<I...>
        ) -> ext::task<Args> {
            co_return Args {
                co_await make_extractor<std::tuple_element_t<I, Args>>(
                    request
                )...
            };
        }

        template <typename... Args>
        auto extract(request& request) {
            return extract<std::tuple<Args...>>(
                request,
                std::index_sequence_for<Args...>()
            );
        }

        template <typename... Args>
        auto use(
            stream& stream,
            std::function<void(Args...)>& fn
        ) -> ext::task<> {
            std::apply(fn, co_await extract<Args...>(stream.request));
            co_return;
        }

        template <response_data R, typename... Args>
        auto use(
            stream& stream,
            std::function<R(Args...)>& fn
        ) -> ext::task<> {
            stream.response.send(std::apply(
                fn,
                co_await extract<Args...>(stream.request)
            ));
            co_return;
        }

        template <typename... Args>
        auto use(
            stream& stream,
            std::function<ext::task<>(Args...)>& fn
        ) -> ext::task<> {
            co_await std::apply(fn, co_await extract<Args...>(stream.request));
        }

        template <response_data R, typename... Args>
        auto use(
            stream& stream,
            std::function<ext::task<R>(Args...)>& fn
        ) -> ext::task<> {
            stream.response.send(co_await std::apply(
                fn,
                co_await extract<Args...>(stream.request))
            );
        }

        template <typename R, typename... Args>
        class handler : public server::handler {
            using function = std::function<R(Args...)>;

            function fn;
        public:
            handler(function&& fn) : fn(std::forward<function>(fn)) {}

            auto handle(stream& stream) -> ext::task<> override {
                co_await use(stream, fn);
            }
        };
    }

    template <typename F>
    auto make_handler(F&& f) -> std::unique_ptr<handler> {
        return std::unique_ptr<handler>(
            new detail::handler(std::function(std::forward<F>(f)))
        );
    }
}
