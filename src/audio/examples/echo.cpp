#include "audio.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <portaudio.h>
#include <unistd.h>

using namespace aud;

int main() {
    initialize();
    mic->dsps.push_back(std::make_shared<RnnoiseDSP>());

    Player p(mic);
    p.start();
    while (1) {
        Pa_Sleep(100);
    }
}