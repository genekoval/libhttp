#include <http/error.h>
#include <http/url.h>

#include <fmt/format.h>
#include <utility>

namespace http {
    url::url() : handle(curl_url()) {
        if (!handle) throw client_error("Failed to allocate URL handle");
    }

    url::url(const char* str) : url() { set(CURLUPART_URL, str); }

    url::url(std::string_view string) : url(string.data()) {}

    url::url(const url& other) : handle(curl_url_dup(other.handle)) {
        if (!handle) throw client_error("Failed to duplicate URL handle");
    }

    url::url(url&& other) : handle(std::exchange(other.handle, nullptr)) {}

    url::~url() { curl_url_cleanup(handle); }

    auto url::operator=(const char* str) -> url& {
        set(CURLUPART_URL, str);
        return *this;
    }

    auto url::operator=(std::string_view string) -> url& {
        set(CURLUPART_URL, string);
        return *this;
    }

    auto url::operator=(const url& other) -> url& {
        if (handle != other.handle) {
            curl_url_cleanup(handle);

            handle = curl_url_dup(other.handle);
            if (!handle) throw client_error("Failed to duplicate URL handle");
        }

        return *this;
    }

    auto url::operator=(url&& other) -> url& {
        if (handle != other.handle) {
            curl_url_cleanup(handle);
            handle = std::exchange(other.handle, nullptr);
        }

        return *this;
    }

    auto url::check_return_code(CURLUcode code) const -> void {
        if (code != CURLUE_OK) throw client_error(curl_url_strerror(code));
    }

    auto url::clear(CURLUPart part) -> void { set(part, nullptr); }

    auto url::data() -> CURLU* { return handle; }

    auto url::fragment() const -> std::optional<http::string> {
        return try_get(CURLUPART_FRAGMENT, CURLUE_NO_FRAGMENT);
    }

    auto url::fragment(std::string_view value) -> void {
        set(CURLUPART_FRAGMENT, value);
    }

    auto url::get(CURLUPart what, unsigned int flags) const -> http::string {
        char* part = nullptr;
        check_return_code(curl_url_get(handle, what, &part, flags));
        return http::string(part);
    }

    auto url::host() const -> std::optional<http::string> {
        return try_get(CURLUPART_HOST, CURLUE_NO_HOST);
    }

    auto url::host(std::string_view value) -> void {
        set(CURLUPART_HOST, value);
    }

    auto url::password() const -> std::optional<http::string> {
        return try_get(CURLUPART_PASSWORD, CURLUE_NO_PASSWORD);
    }

    auto url::password(std::string_view value) -> void {
        set(CURLUPART_PASSWORD, value);
    }

    auto url::path() const -> http::string { return get(CURLUPART_PATH); }

    auto url::path(std::string_view value) -> void {
        set(CURLUPART_PATH, value);
    }

    auto url::port() const -> std::optional<int> {
        if (const auto value = try_get(CURLUPART_PORT, CURLUE_NO_PORT)) {
            return std::strtol(value->data(), nullptr, 10);
        }

        return std::nullopt;
    }

    auto url::port(int value) -> void { port(fmt::to_string(value)); }

    auto url::port(std::string_view value) -> void {
        set(CURLUPART_PORT, value);
    }

    auto url::port_or_default() const -> int {
        const auto value = get(CURLUPART_PORT, CURLU_DEFAULT_PORT);
        return std::strtol(value.data(), nullptr, 10);
    }

    auto url::query() const -> std::optional<http::string> {
        return try_get(CURLUPART_QUERY, CURLUE_NO_QUERY);
    }

    auto url::query(std::string_view value) -> void {
        set(CURLUPART_QUERY, value);
    }

    auto url::scheme() const -> std::optional<http::string> {
        return try_get(CURLUPART_SCHEME, CURLUE_NO_SCHEME);
    }

    auto url::scheme(std::string_view value) -> void {
        set(CURLUPART_SCHEME, value);
    }

    auto url::set(CURLUPart part, const char* content, unsigned int flags)
        -> void {
        check_return_code(curl_url_set(handle, part, content, flags));
    }

    auto url::set(CURLUPart part, std::string_view content, unsigned int flags)
        -> void {
        set(part, content.data(), flags);
    }

    auto url::string() const -> http::string { return get(CURLUPART_URL); }

    auto url::try_get(CURLUPart what, CURLUcode none, unsigned int flags) const
        -> std::optional<http::string> {
        char* part = nullptr;
        const auto code = curl_url_get(handle, what, &part, flags);

        if (code == CURLUE_OK) return http::string(part);
        if (code == none) return std::nullopt;

        throw client_error(curl_url_strerror(code));
    }

    auto url::user() const -> std::optional<http::string> {
        return try_get(CURLUPART_USER, CURLUE_NO_USER);
    }

    auto url::user(std::string_view value) -> void {
        set(CURLUPART_USER, value);
    }

    auto from_json(const nlohmann::json& json, url& url) -> void {
        url = json.get<std::string>();
    }

    auto to_json(nlohmann::json& json, const url& url) -> void {
        json = url.string().data();
    }
}
