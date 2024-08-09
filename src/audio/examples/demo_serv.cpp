#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <iostream>
#include <ostream>
#include <vector>

using namespace boost::asio;

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

    while(1) {
        buf.resize(1024);
        size_t n = sock.receive_from(buffer(buf), addr);
        buf.resize(n);
        std::cout << n << std::endl;
        sock.send_to(buffer(buf), addr);
    }
}