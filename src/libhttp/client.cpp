#include "request.h"

#include <http/http.h>

#include <curl/curl.h>
#include <fmt/format.h>
#include <stdexcept>

namespace http {
    client::client() {
        const auto code = curl_global_init(CURL_GLOBAL_ALL);
        if (code != 0) {
            throw std::runtime_error(fmt::format(
                "cURL failed to initialize: returned ({})",
                code
            ));
        }
    }

    client::~client() { curl_global_cleanup(); }

    auto client::request(const std::string& url) const -> response {
        return request(url, options());
    }

    auto client::request(
        const std::string& url,
        options&& opts
    ) const -> response {
        auto req = internal::request();
        req.url(url);

        if (!opts.body.empty()) req.body(std::move(opts.body));

        for (const auto& header : opts.headers) {
            req.header(header.first, header.second);
        }

        if (opts.method != GET) req.method(opts.method);

        return req.perform();
    }
}
