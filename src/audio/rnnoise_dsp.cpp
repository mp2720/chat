#include "audio.hpp"
#include <cassert>
#include <csignal>
#include <cstdint>
#include <rnnoise.h>

aud::RnnoiseDSP::RnnoiseDSP() {
    assert(aud::SAMPLE_RATE == 48000);
    handler = rnnoise_create(NULL);
    assert(handler);
}

aud::RnnoiseDSP::~RnnoiseDSP() {
    rnnoise_destroy(handler);
}

void aud::RnnoiseDSP::process(float frame[]) {
    int rnnoise_frame_size = rnnoise_get_frame_size();
    assert(FRAME_SIZE % rnnoise_frame_size == 0);
    if (state) {
        for (size_t i = 0; i < FRAME_SIZE; i++) {
            frame[i] *= INT16_MAX;
        }
        for (size_t i = 0; i < FRAME_SIZE / rnnoise_frame_size; i++) {
            rnnoise_process_frame(handler, frame+rnnoise_frame_size*i, frame+rnnoise_frame_size*i);
        }

        for (size_t i = 0; i < FRAME_SIZE; i++) {
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