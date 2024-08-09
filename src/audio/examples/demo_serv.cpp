#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <climits>
#include <iostream>
#include <ostream>
#include <unordered_map>
#include <vector>

using namespace boost::asio;

using timer = std::chrono::system_clock;

int main() {
    std::cout << "Enter the server port:" << std::endl;
    uint16_t port;
    std::cin >> port;

    io_service service;

    ip::udp::socket sock(service);

    sock.open(ip::udp::v4());
    sock.bind(ip::udp::endpoint(ip::udp::v4(), port));
    ip::udp::endpoint addr;
    std::vector<char> buf;

    std::unordered_map<ip::udp::endpoint, decltype(timer::now())> users;

    while (1) {
        buf.resize(1024);
        size_t n = sock.receive_from(buffer(buf), addr);
        buf.resize(n);
        std::cout << "get: " << n << " bytes" << std::endl;
        auto now = timer::now();
        users[addr] = now;



        for (auto it = users.begin(); it != users.end();) {
            if (now - it->second >= std::chrono::seconds(3)) {
                it = users.erase(it);
            } else {
                if (it->first != addr) {
                    sock.send_to(buffer(buf), it->first);
                }
                ++it;
            }
        }
    }
}