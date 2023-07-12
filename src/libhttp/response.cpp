#include <http/http.h>

namespace http {
    status::status(long code) : code(code) {}

    status::operator long() const noexcept {
        return code;
    }

    auto status::ok() const noexcept -> bool {
        return code >= 200 && code <= 299;
    }

    response::response(CURL* handle) : handle(handle) {}

    auto response::content_length() const noexcept -> long {
        curl_off_t result = 0;
        curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &result);
        return result;
    }

    auto response::ok() const noexcept -> bool {
        return this->status().ok();
    }

    auto response::status() const noexcept -> http::status {
        long result = 0;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result);
        return result;
    }
}
