#include <http/client.h>
#include <http/error.h>

#include <curl/curl.h>

namespace http {
    client::client() {
        const auto code = curl_global_init(CURL_GLOBAL_ALL);

        if (code != 0) {
            throw client_error(
                "curl failed to initialize: ({}) {}",
                code,
                curl_easy_strerror(code)
            );
        }
    }

    client::~client() {
        curl_global_cleanup();
    }
}
