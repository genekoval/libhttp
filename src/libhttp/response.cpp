#include <http/http.h>

namespace http {
    response::response(CURL* handle) : handle(handle) {}

    auto response::content_length() const noexcept -> long {
        curl_off_t result = 0;
        curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &result);
        return result;
    }

    auto response::ok() const noexcept -> bool {
        const auto status = this->status();
        return status >= 200 && status <= 299;
    }

    auto response::status() const noexcept -> long {
        long result = 0;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result);
        return result;
    }
}
