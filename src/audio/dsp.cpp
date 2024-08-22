#include "audio.hpp"
#include "log.hpp"
#include <boost/format.hpp>
#include <cassert>
#include <csignal>
#include <cstdint>
#include <rnnoise.h>

aud::RnnoiseDSP::RnnoiseDSP(const char *modelFileName) {
    assert(aud::SAMPLE_RATE == 48000);
    if (modelFileName) {
        FILE *f = fopen(modelFileName, "r");
        if (f) {
            model = rnnoise_model_from_file(f);
            fclose(f);
        } else {
            CHAT_LOGE(
                boost::format("error opening file \"%1%\", rnnoise uses standard model") %
                modelFileName
            );
        }
    }
    handler = rnnoise_create(model);
    assert(handler);
}

aud::RnnoiseDSP::~RnnoiseDSP() {
    rnnoise_destroy(handler);
    if (model) {
        rnnoise_model_free(model);
    }
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
