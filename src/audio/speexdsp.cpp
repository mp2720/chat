#include "speexdsp.hpp"
#include "types.hpp"
#include <speex/speex_preprocess.h>

using namespace chat::aud;
using namespace chat;

SpeexPreprocessor::SpeexPreprocessor() {
    state = speex_preprocess_state_init(FRAME_SIZE, SAMPLE_RATE);
    assert(state != nullptr);
    int disable = 0;
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_DENOISE, &disable);
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_DEREVERB, &disable);
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_ECHO_STATE, NULL);
    speex_preprocess_ctl(state, SPEEX_PREPROCESS_SET_VAD, &disable);
}

bool chat::aud::SpeexPreprocessor::process(const MonoFrame &in, MonoFrame &out) {
    spx_int16_t buf[FRAME_SIZE];
    for (auto i : irange(FRAME_SIZE)) {
        buf[i] = (int16_t)std::lround(in.d[i] * (INT16_MAX));
    }
    bool vad = speex_preprocess_run(state, buf);
    for (auto i : irange(FRAME_SIZE)) {
        out.d[i] = (float)buf[i] / INT16_MAX;
    }
    return vad;
}