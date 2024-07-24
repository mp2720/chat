#include "audio.hpp"

void aud::VolumeDSP::set(float val) {
    this->val = val/100;
}

void aud::VolumeDSP::process(float frame[]) {
    float vl = val;
    for (size_t i = 0; i < FRAME_SIZE; i++) {
        frame[i] *= vl;
    }
}