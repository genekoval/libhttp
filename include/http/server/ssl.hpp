#pragma once

#include <netcore/netcore>

namespace http::server {
    auto ssl() -> netcore::ssl::context;
}
