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

SpeexPreprocessor::~SpeexPreprocessor() {
    speex_preprocess_state_destroy(state);
}

bool SpeexPreprocessor::process(MonoFrame &frame) {
    bool vad = speex_preprocess_run(state, frame.d);
    return vad;
}

void SpeexPreprocessor::control(int request, void *ptr) {
    if (speex_preprocess_ctl(state, request, ptr) == -1) {
        CHAT_LOGE(boost::format("Unknown Speex preprocess request: %1%") % request);
    }
}