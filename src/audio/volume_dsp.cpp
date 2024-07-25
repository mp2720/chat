#include "audio.hpp"

void aud::VolumeDSP::set(float val) {
    this->val = val / 100;
}

void aud::VolumeDSP::process(Frame &frame) {
    float volume = val;
    for (float &v : frame) {
        v *= volume;
    }
}