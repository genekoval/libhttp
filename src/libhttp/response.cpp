#include <http/http.h>

namespace http {
    constexpr auto success_range_start = 200;
    constexpr auto success_range_end = 299;

    response::response(CURL* handle, std::string_view body) :
        body_data(body),
        handle(handle)
    {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
    }

    auto response::body() const noexcept -> std::string_view {
        return body_data;
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
