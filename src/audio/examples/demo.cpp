#include "audio/audio.hpp"
#include "audio/codec.hpp"
#include "audio/rtp.hpp"
#include "audio/rtp_jitter.hpp"
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <cstring>
#include <err.hpp>
#include <iostream>
#include <log.hpp>
#include <memory>
#include <opus.h>
#include <ostream>
#include <portaudio.h>
#include <string>
#include <thread>
#include <vector>

using namespace boost::asio;
using namespace chat;

io_service service;
std::shared_ptr<ip::udp::socket> sock;
ip::udp::endpoint ep;

aud::RTPJitter rjtr(100, aud::SAMPLE_RATE);

void procPack(std::vector<uint8_t> &pack) {
    if (pack.size() < sizeof(aud::RTPHeader)) {
        CHAT_LOGW("Too short a message");
        return;
    }
    aud::rawrtp_ptr p = std::make_shared<aud::RTPPacket>(pack.data(), pack.size());
    rjtr.push(p);
}

void receiver() {
    std::vector<uint8_t> recv_buffer;
    while (1) {
        recv_buffer.resize(1024);
        size_t n = sock->receive(buffer(recv_buffer));
        recv_buffer.resize(n);
        procPack(recv_buffer);
    }
}

int main() {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg) {
            return severity <= Logger::Severity::VERBOSE;
        }
    );
    global_logger.setOutput(&std::cerr);
    err::init();
    aud::initialize();

    std::cout << "Enter the server address:" << std::endl;
    std::string addr;
    std::cin >> addr;

    std::cout << "Enter the server port:" << std::endl;
    uint16_t port;
    std::cin >> port;

    ep = ip::udp::endpoint(ip::address::from_string(addr), port);

    sock = std::make_shared<ip::udp::socket>(service);
    sock->open(ip::udp::v4());
    sock->bind(ip::udp::endpoint(ip::udp::v4(), 0));

    auto out = std::make_shared<aud::RtpOutput>(sock, ep, aud::EncoderPreset::Voise, 1);
    aud::Player p(aud::mic, out);
    p.start();

    std::thread(receiver).detach();

    aud::PaOutput pa(1);
    pa.start();
    aud::rawrtp_ptr pack;
    std::vector<uint8_t> buf;
    aud::Frame fr;
    fr.resize(aud::FRAME_SIZE);

    int err;
    OpusDecoder *dec = opus_decoder_create(aud::SAMPLE_RATE, 1, &err);
    if (err != OPUS_OK) {
        throw aud::OpusException(err);
    }

    while (1) {
        aud::RTPJitter::RESULT res;
        do {
            Pa_Sleep(10);
            res = rjtr.pop(pack);
        } while (res == aud::RTPJitter::RESULT::BUFFERING);
        
        if (res == aud::RTPJitter::RESULT::SUCCESS) {
            size_t size = pack->nLen - sizeof(aud::RTPHeader);
            buf.resize(size);
            uint8_t *d = pack->pData + sizeof(aud::RTPHeader);
            memcpy(buf.data(), d, size);
        } else {
            buf.resize(0);
        }
        err = opus_decode_float(dec, buf.data(), buf.size(), fr.data(), fr.size(), 0);
        if (err < 0) {
            throw aud::OpusException(err);
        }
        pa.write(fr);
    }

    aud::terminate();
    opus_decoder_destroy(dec);
}