#include <http/string.h>

#include <curl/curl.h>

namespace http {
    string::string() : data(nullptr) {}

    string::string(char* data) : data(data) {}

    string::~string() {
        curl_free(data);
    }

    string::operator std::string_view() const noexcept {
        return std::string_view(data);
    }
}
