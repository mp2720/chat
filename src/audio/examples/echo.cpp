#include "audio/audio.hpp"
#include "log.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
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
    initialize();
    mic->dsps.push_back(std::make_shared<RnnoiseDSP>());
    auto po = std::make_shared<PaOutput>(mic->channels());
    Player p(mic, po);
    p.start();
    while (1) {
        Pa_Sleep(100);
    }
}
