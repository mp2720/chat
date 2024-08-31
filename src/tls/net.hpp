#pragma once

#include <boost/core/span.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mbedtls/cipher.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net_sockets.h>
#include <string>

namespace chat {

class NetClient {
  public:
    enum class Proto {
        TCP = MBEDTLS_NET_PROTO_TCP,
        UDP = MBEDTLS_NET_PROTO_UDP,
    };
    NetClient(const std::string &host, const std::string &port, Proto protocol);
    ~NetClient();

    mbedtls_net_context *handler() {
        return &ctx;
    }

    size_t send(boost::span<uint8_t> data);
    size_t recv(boost::span<uint8_t> data);
    size_t recvTimeout(boost::span<uint8_t> data, std::chrono::milliseconds timeout);

    static int callbackSend(void *ctx, const unsigned char *buf, size_t len) {
        return mbedtls_net_send(&((NetClient *)ctx)->ctx, buf, len);
    }

    static int callbackRecv(void *ctx, unsigned char *buf, size_t len) {
        return mbedtls_net_recv(&((NetClient *)ctx)->ctx, buf, len);
    }

    static int callbackRecvTimout(void *ctx, unsigned char *buf, size_t len, uint32_t timeout) {
        return mbedtls_net_recv_timeout(&((NetClient *)ctx)->ctx, buf, len, timeout);
    }

  private:
    mbedtls_net_context ctx;
};

class Random {
  public:
    Random(const std::string &pers = "");
    ~Random();
    void rand(boost::span<uint8_t> block);

    static int callback(void *p_rng, unsigned char *output, size_t output_len) {
        return mbedtls_ctr_drbg_random(&((Random *)p_rng)->ctr_drbg, output, output_len);
    }

  private:
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
};

} // namespace chat