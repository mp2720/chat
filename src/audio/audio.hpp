#pragma once

#include "defs.hpp"
#include "rnnoise.hpp"
#include "speexdsp.hpp"
#include <atomic>
#include <cstdio>
#include <mutex>
#include <rnnoise.h>
#include <vector>

namespace chat {
namespace aud {

class Audio {
  public:
    Audio(const Audio &) = delete;
    Audio &operator=(const Audio &) = delete;
    void addCallback(Callback cb);

    PaStream *getStream() {
        return stream;
    }

    SpeexPreprocessor &getDsp() {
        return speexPrepr;
    }

    static Audio &instance() {
        static Audio audio;
        return audio;
    }

    enum class DenoiseType { None, Rnnoise, Speex };

    void setDenoise(DenoiseType type) {
        denoiseType = type;
    }

  private:
    Audio();
    ~Audio();

    RnnoiseDenoiser rnnoiseDnsr;
    SpeexPreprocessor speexPrepr;

    PaStream *stream;
    int callback(const MonoFrame &input, Frame &output) noexcept;
    void cbDenoise(MonoFrame &frame);
    std::vector<Callback> callbacks;
    std::vector<Callback> cbsToAdd;
    std::mutex lock;
    Frame outputBuf;
    MonoFrame inputBuf;
    std::atomic<DenoiseType> denoiseType = DenoiseType::None;
    DenoiseType lastDenoiseType = DenoiseType::None;
};

} // namespace aud
} // namespace chat