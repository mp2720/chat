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

void aud::RnnoiseDSP::process(Frame &frame) {
    int rnnoise_frame_size = rnnoise_get_frame_size();
    assert(
        frame.size() % rnnoise_frame_size == 0 && "frame size must be a multiple of rnnoise size"
    );

    if (state) {
        for (float &v : frame) {
            v *= INT16_MAX;
        }
        for (size_t i = 0; i < FRAME_SIZE / rnnoise_frame_size; i++) {
            rnnoise_process_frame(
                handler,
                frame.data() + rnnoise_frame_size * i,
                frame.data() + rnnoise_frame_size * i
            );
        }
        for (float &v : frame) {
            v *= 1.f / INT16_MAX;
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

void aud::VolumeDSP::set(float val) {
    this->val = val / 100;
}

void aud::VolumeDSP::process(Frame &frame) {
    float volume = val;
    for (float &v : frame) {
        v *= volume;
    }
}