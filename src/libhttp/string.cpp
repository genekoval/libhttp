#include <http/string.h>

#include <curl/curl.h>
#include <utility>

namespace http {
    string::string() : str(nullptr) {}

    string::string(char* data) : str(data) {}

    string::string(string&& other) : str(std::exchange(other.str, nullptr)) {}

    string::~string() { curl_free(str); }

    string::operator std::string_view() const noexcept {
        return std::string_view(str);
    }

    auto string::operator=(string&& other) -> string& {
        if (std::addressof(other) != this) {
            curl_free(str);
            str = std::exchange(other.str, nullptr);
        }

        return *this;
    }

    auto string::data() const noexcept -> const char* { return str; }
}
