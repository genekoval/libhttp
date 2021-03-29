#include "request.h"

#include <array>
#include <fmt/format.h>
#include <stdexcept>

namespace http::internal {
    constexpr auto methods = std::array<CURLoption, 1> {
        CURLOPT_HTTPGET
    };

    static auto write_callback(
        char* ptr,
        size_t size,
        size_t nmemb,
        void* userdata
    ) -> size_t {
        auto* res = reinterpret_cast<response*>(userdata);
        res->receive(std::string_view(ptr, nmemb));

        return nmemb;
    }

    request::request() : handle(curl_easy_init()) {
        if (!handle) {
            throw std::runtime_error("failed to create cURL handle");
        }
    }

    auto request::method(int mtd) -> void {
        set(methods[mtd], 1L);
    }

    auto request::perform() -> response {
        auto res = response(handle);

        set(CURLOPT_WRITEFUNCTION, write_callback);
        set(CURLOPT_WRITEDATA, &res);

        const auto code = curl_easy_perform(handle);

        if (code != CURLE_OK) {
            throw std::runtime_error(fmt::format(
                "cURL request failure: returned ({})",
                code
            ));
        }

        return res;
    }

    auto request::url(const std::string& url_string) -> void {
        set(CURLOPT_URL, url_string.c_str());
    }
}
