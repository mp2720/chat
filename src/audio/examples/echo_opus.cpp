#include "audio.hpp"
#include "codec.cpp"

#include "audio.hpp"
#include "codec.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <portaudio.h>
#include <unistd.h>

using namespace aud;

int main() {
    initialize();
    //mic->dsps.push_back(std::make_shared<RnnoiseDSP>());
    shared_ptr<Encoder> enc = std::make_shared<Encoder>(mic, EncoderPreset::Voise);
    shared_ptr<Decoder> dec = std::make_shared<Decoder>(enc);
    Player p(dec);
    p.start();
    while (1) {
        Pa_Sleep(1000);
    }
}