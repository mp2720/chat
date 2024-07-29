#include "audio/audio.hpp"
#include "err.hpp"
#include "log.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <portaudio.h>
#include <unistd.h>

using namespace aud;
using namespace chat;
int main() {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg) {
            return severity <= Logger::Severity::VERBOSE;
        }
    );
    global_logger.setOutput(&std::cerr);
    err::init();
    initialize();
    mic->dsps.push_back(std::make_shared<RnnoiseDSP>());

    Player p(mic);
    p.start();
    while (1) {
        Pa_Sleep(100);
    }
}