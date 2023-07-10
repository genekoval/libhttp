#include <http/http.h>

namespace http {
    constexpr auto success_range_start = 200;
    constexpr auto success_range_end = 299;

    response::response(CURL* handle) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
    }

    auto response::ok() const noexcept -> bool {
        return
            response_code >= success_range_start &&
            response_code <= success_range_end;
    }

    auto response::status() const noexcept -> long {
        return response_code;
    }
}
