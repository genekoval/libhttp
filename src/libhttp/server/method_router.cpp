#include <http/server/method_router.hpp>

namespace http::server {
    auto method_router::allowed() -> std::string_view {
        if (cache_invalid) {
            allowed_cache = std::as_const(*this).allowed();
            cache_invalid = false;
        }

        return allowed_cache;
    }

    auto method_router::allowed() const -> std::string {
        auto methods = std::vector<std::string_view>();
        methods.reserve(this->methods.size());

        for (const auto& entry : this->methods) {
            methods.push_back(entry.first);
        }

        std::sort(methods.begin(), methods.end());

        return fmt::format("{}", fmt::join(methods, ", "));
    }

    auto method_router::find(std::string_view method) -> handler* {
        auto result = methods.find(method);

        if (result == methods.end()) return nullptr;
        return result->second.get();
    }
}
