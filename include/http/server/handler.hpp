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
        auto use(stream& stream, void (*fn)(Args...)) -> ext::task<> {
            std::apply(fn, co_await extract<Args...>(stream.request));
            co_return;
        }

        template <response_data R, typename... Args>
        auto use(stream& stream, R (*fn)(Args...)) -> ext::task<> {
            stream.response.send(std::apply(
                fn,
                co_await extract<Args...>(stream.request)
            ));
            co_return;
        }

        template <typename... Args>
        auto use(stream& stream, ext::task<> (*fn)(Args...)) -> ext::task<> {
            co_await std::apply(fn, co_await extract<Args...>(stream.request));
        }

        template <response_data R, typename... Args>
        auto use(stream& stream, ext::task<R> (*fn)(Args...)) -> ext::task<> {
            stream.response.send(co_await std::apply(
                fn,
                co_await extract<Args...>(stream.request))
            );
        }

        template <typename T, typename U, typename... Args>
        auto use(
            stream& stream,
            std::tuple<std::reference_wrapper<T>> t,
            void (U::* fn)(Args...)
        ) -> ext::task<> {
            std::apply(fn, std::tuple_cat(
                t,
                co_await extract<Args...>(stream.request)
            ));
            co_return;
        }

        template <typename T, typename U, response_data R, typename... Args>
        auto use(
            stream& stream,
            std::tuple<std::reference_wrapper<T>> t,
            R (U::* fn)(Args...)
        ) -> ext::task<> {
            stream.response.send(std::apply(fn, std::tuple_cat(
                t,
                co_await extract<Args...>(stream.request)
            )));
            co_return;
        }

        template <typename T, typename U, typename... Args>
        auto use(
            stream& stream,
            std::tuple<std::reference_wrapper<T>> t,
            ext::task<> (U::* fn)(Args...)
        ) -> ext::task<> {
            co_await std::apply(fn, std::tuple_cat(
                t,
                co_await extract<Args...>(stream.request)
            ));
        }

        template <typename T, typename U, response_data R, typename... Args>
        auto use(
            stream& stream,
            std::tuple<std::reference_wrapper<T>> t,
            ext::task<R> (U::* fn)(Args...)
        ) -> ext::task<> {
            stream.response.send(co_await std::apply(fn, std::tuple_cat(
                t,
                co_await extract<Args...>(stream.request)
            )));
        }

        template <typename F>
        class handler_fn : public handler {
            F f;
        public:
            handler_fn(F&& f) : f(std::forward<F>(f)) {}

            auto handle(stream& stream) -> ext::task<> override {
                co_await use(stream, f);
            }
        };

        template <typename T, typename F>
        class handler_member : public handler {
            std::tuple<std::reference_wrapper<T>> t;
            F f;
        public:
            handler_member(T& t, F f) : t(t), f(f) {}

            auto handle(stream& stream) -> ext::task<> override {
                co_await use(stream, t, f);
            };
        };
    }

    template <typename F>
    auto make_handler(F&& f) -> std::unique_ptr<handler> {
        return std::unique_ptr<handler>(
            new detail::handler_fn<F>(std::forward<F>(f))
        );
    }

    template <typename T, typename F>
    auto make_handler(T& t, F f) -> std::unique_ptr<handler> {
        return std::unique_ptr<handler>(
            new detail::handler_member<T, F>(t, f)
        );
    }
}
