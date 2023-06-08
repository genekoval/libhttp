#pragma once

#include "method_router.hpp"
#include "node.hpp"

namespace http::server {
    using path = node<method_router>;

    class router {
        path paths;
    public:
        router(path&& paths);

        auto route(stream& stream) -> ext::task<bool>;
    };
}
