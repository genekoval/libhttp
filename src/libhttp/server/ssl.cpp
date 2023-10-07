#include <http/server/ssl.hpp>

#include <nghttp2/nghttp2.h>

namespace {
    constexpr auto next_proto_list =
        std::array<const unsigned char, 3> {2, 'h', '2'};

    auto alpn_select(
        SSL* ssl,
        const unsigned char** out,
        unsigned char* outlen,
        const unsigned char* in,
        unsigned int inlen,
        void* arg
    ) -> int {
        const auto ret = nghttp2_select_next_protocol(
            (unsigned char**) out,
            outlen,
            in,
            inlen
        );

        if (ret != 1) return SSL_TLSEXT_ERR_NOACK;
        return SSL_TLSEXT_ERR_OK;
    }

    auto next_protos_advertised(
        SSL* ssl,
        const unsigned char** data,
        unsigned int* len,
        void* arg
    ) -> int {
        *data = next_proto_list.data();
        *len = next_proto_list.size();

        return SSL_TLSEXT_ERR_OK;
    }
}

namespace http::server {
    auto ssl() -> netcore::ssl::context {
        auto ssl = netcore::ssl::context::server();

        ssl.set_next_protos_advertised_callback(next_protos_advertised);
        ssl.set_alpn_select_callback(alpn_select);

        return ssl;
    }
}
