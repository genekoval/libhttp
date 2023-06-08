#pragma once

#include <fmt/format.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace http::server {
    template <typename T>
    struct match {
        T* value;
        std::unordered_map<std::string_view, std::string_view> params;
    };

    enum class node_type {
        static_route,
        param,
        catch_all
    };

    template <typename T>
    class node {
        node_type type;
        std::string prefix;
        std::optional<T> value;
        std::vector<node> children;

        node(std::string_view prefix) : prefix(prefix) {}

        node(
            std::string_view prefix,
            std::optional<T>&& value,
            std::vector<node>&& children
        ) :
            prefix(prefix),
            value(std::forward<std::optional<T>>(value)),
            children(std::forward<std::vector<node>>(children))
        {}

        auto assign(std::string_view route, T&& value) -> node& {
            auto begin = route.begin();
            const auto end = route.end();

            auto it = begin;

            auto* current = this;

            while (it != end) {
                const auto c = *it;
                if (c != ':' && c != '*') {
                    ++it;
                    continue;
                }

                if (it != begin) {
                    current->prefix = {begin, it};
                    current = &current->children.emplace_back();
                }

                begin = ++it;

                while (it != end && *it != '/') ++it;
                current->prefix = {begin, it};

                begin = it;

                current->type = c == ':' ?
                    node_type::param : node_type::catch_all;

                if (current->type == node_type::catch_all && it != end) {
                    throw std::runtime_error("invalid catch-all");
                }

                if (it != end) current = &current->children.emplace_back();
            }

            if (begin != end) current->prefix = {begin, it};
            current->value = std::forward<T>(value);

            return *current;
        }

        auto find(
            std::string_view route,
            std::unordered_map<std::string_view, std::string_view>& params
        ) -> T* {
            switch (type) {
                case node_type::static_route:
                    if (!route.starts_with(prefix)) return nullptr;
                    route = route.substr(prefix.size());
                    break;
                case node_type::param: {
                    const auto slash = route.find('/');
                    params.insert({prefix, route.substr(0, slash)});
                    route = slash != std::string_view::npos ?
                        route.substr(slash) : std::string_view();
                    break;
                }
                case node_type::catch_all:
                    params.insert({prefix, route});
                    return &*value;
            }

            if (route.empty()) return value ? &*value : nullptr;

            for (auto& child : children) {
                if (auto* const result = child.find(route, params)) {
                    return result;
                }
            }

            return nullptr;
        }

        auto format_to(
            std::back_insert_iterator<fmt::memory_buffer>& out,
            int level
        ) const -> void {
            auto type = std::string_view();

            switch (this->type) {
                case node_type::static_route: type = ""; break;
                case node_type::param: type = ":"; break;
                case node_type::catch_all: type = "*"; break;
            }

            const auto indent = level * 2;
            for (auto i = 0; i < indent; ++i) fmt::format_to(out, " ");

            fmt::format_to(
                out,
                "{}{} {}\n",
                type,
                prefix,
                value ? fmt::to_string(*value) : ""
            );

            for (const auto& child : children) child.format_to(out, level + 1);
        }
    public:
        node() = default;

        auto find(std::string_view route) -> std::optional<match<T>> {
            if (route.size() > 1 && route.ends_with('/')) {
                route = {route.begin(), route.end() - 1};
            }

            auto params =
                std::unordered_map<std::string_view, std::string_view>();

            if (auto* const result = find(route, params)) {
                return match<T> {
                    .value = result,
                    .params = std::move(params)
                };
            }

            return std::nullopt;
        }

        auto insert(node& child) -> node& {
            return insert(std::move(child));
        }

        auto insert(node&& child) -> node& {
            auto it = children.begin();

            while (
                it != children.end() &&
                it->type == node_type::static_route
            ) ++it;

            children.insert(it, std::forward<node>(child));
            return *this;
        }

        auto insert(std::string_view route, T& value) -> node& {
            return insert(route, std::move(value));
        }

        auto insert(std::string_view route, T&& value) -> node& {
            if (prefix.empty()) {
                assign(route, std::forward<T>(value));
                return *this;
            }

            auto* current = this;

            while (true) {
                if (
                    (route.starts_with(':') || route.starts_with('*')) &&
                    current->type != node_type::static_route
                ) {
                    route = route.substr(1);
                    const auto slash = route.find('/');

                    const auto key = route.substr(0, slash);

                    if (key != current->prefix) {
                        throw std::runtime_error(fmt::format(
                            "param collision {} -> {}",
                            current->prefix,
                            key
                        ));
                    }

                    route = route.substr(current->prefix.size());
                    if (route.empty()) {
                        current->value = std::forward<T>(value);
                        return *current;
                    }
                }
                else {
                    auto i = 0;
                    const auto max = std::min(
                        route.size(),
                        current->prefix.size()
                    );

                    while (i < max && route[i] == current->prefix[i]) ++i;

                    auto split = false;

                    if (i < current->prefix.size()) {
                        auto child = node(
                            current->prefix.substr(i),
                            std::exchange(current->value, {}),
                            std::exchange(current->children, {})
                        );

                        current->children.emplace_back(std::move(child));
                        current->prefix = route.substr(0, i);

                        split = true;
                    }

                    if (i == route.size()) {
                        current->value = std::forward<T>(value);
                        return *current;
                    }

                    route = route.substr(i);

                    if (split) {
                        return current->children.emplace_back().assign(
                            route,
                            std::forward<T>(value)
                        );
                    }
                }

                auto found = false;
                const auto first = route[0];
                for (auto& child : current->children) {
                    if (
                        (
                            (first == ':' || first == '*') &&
                            child.type != node_type::static_route
                        ) || (child.prefix[0] == first)
                    ) {
                        current = &child;
                        found = true;
                        break;
                    }
                }
                if (found) continue;

                auto child = node();
                auto& result = child.assign(route, std::forward<T>(value));

                const auto nested = &child != &result;

                if (child.type != node_type::static_route) {
                    auto& emplaced =
                        current->children.emplace_back(std::move(child));
                    if (!nested) return emplaced;
                }
                else {
                    auto it = current->children.begin();
                    while (
                        it != current->children.end() &&
                        it->type == node_type::static_route
                    ) ++it;
                    auto inserted =
                        current->children.insert(it, std::move(child));
                    if (!nested) return *inserted;
                }

                return result;
            }
        }

        auto to_string() const -> std::string {
            auto buffer = fmt::memory_buffer();
            auto out = std::back_inserter(buffer);

            format_to(out, 0);

            return fmt::to_string(buffer);
        }
    };
}
