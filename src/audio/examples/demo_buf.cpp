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
#include <utility>
#include <vector>

using namespace boost::asio;
using namespace chat;

io_service service;
std::shared_ptr<ip::udp::socket> sock;
ip::udp::endpoint ep;

void receiver(aud::NetBuf *nb) {
    sock->bind(ip::udp::endpoint(ip::udp::v4(), 0));
    std::vector<uint8_t> recv_buffer;

    aud::Frame frame;
    frame.resize(aud::FRAME_SIZE);

    while (1) {
        recv_buffer.resize(1024);
        size_t n = sock->receive(buffer(recv_buffer));
        recv_buffer.resize(n);
        nb->push(std::move(recv_buffer));
    }
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

    aud::NetBuf nb;

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
    std::thread(receiver, &nb).detach();

    aud::PaOutput out(1);
    out.start();
    aud::Frame frame;
    while (1) {
        nb.pop(frame);
        out.write(frame);
    }
}