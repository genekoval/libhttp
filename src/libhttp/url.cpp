#include <http/error.h>
#include <http/url.h>

namespace http {
    url::url() : handle(curl_url()) {
        if (!handle) {
            throw http::client_error("failed to allocate URL handle");
        }
    }

    url::url(const url& other) : handle(curl_url_dup(other.handle)) {
        if (!handle) {
            throw http::client_error("failed to duplicate URL handle");
        }
    }

    url::~url() {
        curl_url_cleanup(handle);
        curl_free(part);
    }

    auto url::check_return_code(CURLUcode code) -> void {
        if (code != CURLUE_OK) {
            throw client_error(
                "URL error ({}): {}",
                code,
                curl_url_strerror(code)
            );
        }
    }

    auto url::clear(CURLUPart part) -> void {
        set(part, nullptr);
    }

    auto url::data() -> CURLU* {
        return handle;
    }

    auto url::get(CURLUPart what, unsigned int flags) -> std::string_view {
        curl_free(part);
        check_return_code(curl_url_get(handle, what, &part, flags));
        return part;
    }

    auto url::set(
        CURLUPart part,
        const char* content,
        unsigned int flags
    ) -> void {
        check_return_code(curl_url_set(handle, part, content, flags));
    }

    auto url::set(
        CURLUPart part,
        const std::string& content,
        unsigned int flags
    ) -> void {
        set(part, content.data(), flags);
    }
}
