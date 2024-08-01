#include "audio/audio.hpp"
#include "audio/codec.hpp"
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
    shared_ptr<Encoder> enc = std::make_shared<Encoder>(mic, EncoderPreset::Voise);
    shared_ptr<Decoder> dec = std::make_shared<Decoder>(enc);
    Player p(dec);
    p.start();
    while (1) {
        Pa_Sleep(1000);
    }
}
