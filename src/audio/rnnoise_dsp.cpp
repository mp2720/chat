#include "audio.hpp"
#include <cassert>
#include <csignal>
#include <cstdint>
#include <rnnoise.h>

aud::RnnoiseDSP::RnnoiseDSP() {
    handler = rnnoise_create(NULL);
    assert(handler);
}

aud::RnnoiseDSP::~RnnoiseDSP() {
    rnnoise_destroy(handler);
}

void aud::RnnoiseDSP::process(float frame[]) {
    if (state) {
        size_t frameSize = aud::Recorder::getFrameSize();
        for (size_t i = 0; i < frameSize; i++) {
            frame[i] *= INT16_MAX;
        }
        rnnoise_process_frame(handler, frame, frame);
        for (size_t i = 0; i < frameSize; i++) {
            frame[i] *= 1.f / INT16_MAX;
        }
    }
}

void aud::RnnoiseDSP::on() {
    state = true;
}

void aud::RnnoiseDSP::off() {
    state = false;
}

bool aud::RnnoiseDSP::getState() {
    return state;
}