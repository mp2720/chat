#include "audio.hpp"
#include "audio/defs.hpp"
#include "types.hpp"
#include <boost/format.hpp>
#include <boost/range/irange.hpp>
#include <cassert>
#include <cstring>
#include <mutex>
#include <portaudio.h>
#include <rnnoise.h>
#include <speex/speex_preprocess.h>

using namespace chat::aud;
using namespace chat;

// automatic check portaudio functions for errors
template <typename Func, typename... Args>
static auto safePa(Func func, Args... args) -> decltype(func(args...)) {
    auto res = func(args...);
    if (res < 0) {
        throw Exception(res);
    }
    return res;
}

void Audio::addCallback(Callback cb) {
    std::lock_guard lg(lock);
    cbsToAdd.push_back(cb);
    if (safePa(Pa_IsStreamStopped, stream)) {
        safePa(Pa_StartStream, stream);
    }
}

Audio::Audio() {
    safePa(Pa_Initialize);
    safePa(
        Pa_OpenDefaultStream,
        &stream,
        1,
        CHANNELS,
        paFloat32,
        SAMPLE_RATE,
        FRAME_SIZE,
        [](const void *inputBuffer,
           void *outputBuffer,
           unsigned long framesPerBuffer,
           const PaStreamCallbackTimeInfo *timeInfo,
           PaStreamCallbackFlags statusFlags,
           void *userData) noexcept {
            assert(framesPerBuffer == FRAME_SIZE);
            (void)timeInfo;
            (void)statusFlags;
            return static_cast<Audio *>(userData)->callback(
                *static_cast<const MonoFrame *>(inputBuffer),
                *static_cast<Frame *>(outputBuffer)
            );
        },
        this
    );
}

Audio::~Audio() {
    // no check errors in destructor
    Pa_CloseStream(stream);
    Pa_Terminate();
}

Frame::Frame(const MonoFrame &mframe) {
    for (auto i : irange(FRAME_SIZE)) {
        for (auto ch : irange(CHANNELS)) {
            d[i][ch] = mframe.d[i];
        }
    }
}

MonoFrame::MonoFrame(const Frame &frame) {
    for (auto i : irange(FRAME_SIZE)) {
        for (auto ch : irange(CHANNELS)) {
            d[i] += frame.d[i][ch];
        }
        d[i] /= CHANNELS;
    }
}