#include "audio/audio.hpp"
#include "audio/codec.hpp"
#include "err.hpp"
#include "log.hpp"
#include "rtp.hpp"
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/format/format_fwd.hpp>
#include <cstring>
#include <memory>
#include <netinet/in.h>

using namespace aud;

RtpOutput::RtpOutput(std::shared_ptr<ip::udp::socket> conn, ip::udp::endpoint addr, EncoderPreset ep, int channels)
    : conn(conn), enc(ep, channels), chans(channels), addr(addr) {
    CHAT_ASSERT(conn);
}

void RtpOutput::start() {}

void RtpOutput::stop() {}

int RtpOutput::channels() const {
    return chans;
}

void RtpOutput::write(Frame &frame) {
    assert(!frame.empty());
    enc.encode(frame, encBuf);

    rtpBuf.resize(sizeof(RTPHeader)+encBuf.size());
    auto *v = reinterpret_cast<char *>(rtpBuf.data());
    auto *h = reinterpret_cast<RTPHeader *>(v);
    h->flags = htons(0x8000); //2 version
    h->sequence = htons(sequence);
    h->timestamp = htonl(sequence * FRAME_SIZE);
    h->ssrc = 0;
    sequence += 1;
    v += sizeof(RTPHeader);
    memcpy(v, encBuf.data(), encBuf.size());
    size_t sended = conn->send_to(buffer(rtpBuf), addr);
    if (sended != rtpBuf.size()) {
        CHAT_LOGW(boost::format("Sent less than planned %1%/%2%") % sended % rtpBuf.size());
    }
}