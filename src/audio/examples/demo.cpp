#include "audio/audio.hpp"
#include "audio/codec.hpp"
#include "opus_defines.h"
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <log.hpp>
#include <memory>
#include <opus.h>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

using namespace boost::asio;
using namespace chat;

io_service service;
std::shared_ptr<ip::udp::socket> sock;
ip::udp::endpoint ep;

void receiver() {
    sock->bind(ip::udp::endpoint(ip::udp::v4(), 0));
    std::vector<uint8_t> recv_buffer;

    int err;
    OpusDecoder *dec = opus_decoder_create(aud::SAMPLE_RATE, 1, &err);
    if (err < 0) {
        throw aud::OpusException(err);
    }
    aud::Frame frame;
    frame.resize(aud::FRAME_SIZE);

    aud::PaOutput out(1);
    out.start();
    while (1) {
        recv_buffer.resize(1024);
        size_t n = sock->receive(buffer(recv_buffer));
        recv_buffer.resize(n);
        err = opus_decode_float(
            dec,
            recv_buffer.data(),
            recv_buffer.size(),
            frame.data(),
            frame.size(),
            0
        );
        if (err < 0) {
            CHAT_LOGW(opus_strerror(err));
        }
        out.write(frame);
    }
    opus_decoder_destroy(dec);
}

void sender() {
    std::vector<uint8_t> send_buffer;
    aud::OpusEncSrc es(aud::mic, aud::EncoderPreset::Voise);
    es.start();
    while (1) {
        try {
            es.encode(send_buffer);
        } catch (aud::OpusException &ex) {
            CHAT_LOGW(ex.ErrorText());
        }
        sock->send_to(buffer(send_buffer), ep);
    }
}

int main() {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg) {
            return severity <= Logger::Severity::VERBOSE;
        }
    );
    global_logger.setOutput(&std::cerr);
    aud::initialize();

    aud::mic->dsps.push_back(std::make_shared<aud::RnnoiseDSP>());

    std::cout << "Enter the server address:" << std::endl;
    std::string addr;
    std::cin >> addr;

    std::cout << "Enter the server port:" << std::endl;
    uint16_t port;
    std::cin >> port;

    ep = ip::udp::endpoint(ip::address::from_string(addr), port);

    sock = std::make_shared<ip::udp::socket>(service);
    sock->open(ip::udp::v4());

    std::thread(sender).detach();
    receiver();
}