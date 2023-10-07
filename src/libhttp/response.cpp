#include <http/response.h>

namespace http {
    status::status(long code) : code(code) {}

    status::operator long() const noexcept { return code; }

    auto status::ok() const noexcept -> bool {
        return code >= 200 && code <= 299;
    }

    response::response(CURL* handle, std::string&& body) :
        handle(handle),
        body(std::forward<std::string>(body)) {}

    auto response::content_length() const noexcept -> long {
        curl_off_t result = 0;
        curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &result);
        return result;
    }

    auto response::content_type() const -> std::optional<media_type> {
        if (const auto value = header("content-type")) return *value;
        return std::nullopt;
    }

    auto response::data() const& noexcept -> std::string_view { return body; }

    auto response::data() && noexcept -> std::string { return std::move(body); }

    auto response::header(std::string_view name) const
        -> std::optional<std::string_view> {
        curl_header* header = nullptr;

        const auto code =
            curl_easy_header(handle, name.data(), 0, CURLH_HEADER, -1, &header);

        if (code == CURLHE_OK) return header->value;

        return std::nullopt;
    }

    auto response::ok() const noexcept -> bool { return this->status().ok(); }

    auto response::status() const noexcept -> http::status {
        long result = 0;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result);
        return result;
    }
}
