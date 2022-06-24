#include <http/request.h>

#include <timber/timber>

namespace http {
    method_guard::method_guard(http::request* request) : request(request) {}

    method_guard::~method_guard() {
        try {
            request->set(CURLOPT_CUSTOMREQUEST, nullptr);
        }
        catch (const client_error& ex) {
            TIMBER_ERROR(
                "failed to reset custom request method: {}",
                ex.what()
            );
        }
    }
}
