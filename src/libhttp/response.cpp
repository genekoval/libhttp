#include <http/http.h>

namespace http {
    constexpr auto success_range_start = 200;
    constexpr auto success_range_end = 299;

    response::response(CURL* handle) : handle(handle) {}

    response::~response() {
        curl_easy_cleanup(handle);
    }

    auto response::ok() -> bool {
        const auto code = status();
        return code >= success_range_start && code <= success_range_end;
    }

    auto response::status() -> long {
        auto code = 0L;
        get(CURLINFO_RESPONSE_CODE, &code);
        return code;
    }
}
