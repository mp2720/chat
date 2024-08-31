#include "net.hpp"
#include "err.hpp"
#include "log.hpp"
#include <boost/format/format_fwd.hpp>
#include <mbedtls/net_sockets.h>

using namespace chat;

NetClient::NetClient(const std::string &host, const std::string &port, Proto protocol) {
    mbedtls_net_init(&ctx);
    int ret = mbedtls_net_connect(&ctx, host.c_str(), port.c_str(), static_cast<int>(protocol));
    if (ret < 0) {
        throw MbedtlsException(ret);
    } else {
        CHAT_LOGV(boost::format("Connected to %1%:%2%") % host % port);
    }
}

NetClient::~NetClient() {
    mbedtls_net_close(&ctx);
    mbedtls_net_free(&ctx);
}

size_t NetClient::send(boost::span<uint8_t> data) {
    int ret = NetClient::callbackSend(this, data.data(), data.size_bytes());
    if (ret < 0)
        throw MbedtlsException(ret);
    return ret;
}

size_t NetClient::recv(boost::span<uint8_t> data) {
    int ret = NetClient::callbackRecv(this, data.data(), data.size_bytes());
    if (ret < 0)
        throw MbedtlsException(ret);
    return ret;
}

size_t NetClient::recvTimeout(boost::span<uint8_t> data, std::chrono::milliseconds timeout) {
    int ret = NetClient::callbackRecvTimout(this, data.data(), data.size_bytes(), timeout.count());
    if (ret < 0)
        throw MbedtlsException(ret);
    return ret;
}

Random::Random(const std::string &pers) {
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    int ret = mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char *)pers.data(),
        pers.size()
    );
    if (ret < 0)
        throw MbedtlsException(ret);
}

Random::~Random() {
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

void Random::rand(boost::span<uint8_t> block) {
    int ret = Random::callback(this, block.data(), block.size_bytes());
    if (ret < 0)
        throw MbedtlsException(ret);
}
