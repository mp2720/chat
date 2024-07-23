#include "audio.hpp"

void aud::VolumeDSP::set(float val) {
    this->val = val/100;
}

void aud::VolumeDSP::process(float frame[]) {
    float vl = val;
    size_t n = aud::Recorder::getFrameSize();
    for (size_t i = 0; i < n; i++) {
        frame[i] *= vl;
    }
}