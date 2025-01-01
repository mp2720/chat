#include "audio.hpp"
#include "types.hpp"
#include "log.hpp"

using namespace chat::aud;
using namespace chat;

static void compress(Frame &frame) {
    float maxVal = 0;
    for (auto i : irange(FRAME_SIZE)) {
        for (auto ch : irange(CHANNELS)) {
            maxVal = std::max(maxVal, std::abs(frame.d[i][ch]));
        }
    }
    if (maxVal > 1) {
        float k = 1 / maxVal;
        for (auto i : irange(FRAME_SIZE)) {
            for (auto ch : irange(CHANNELS)) {
                frame.d[i][ch] *= k;
            }
        }
    }
}

void Audio::cbDenoise(const MonoFrame &input) {
    auto dtype = denoiseType.load();

    if (dtype == DenoiseType::Speex && lastDenoiseType != DenoiseType::Speex) {
        int enable = 1;
        speexPrepr.control(SPEEX_PREPROCESS_SET_DENOISE, &enable);

    } else if (dtype != DenoiseType::Speex && lastDenoiseType == DenoiseType::Speex) {
        int disable = 0;
        speexPrepr.control(SPEEX_PREPROCESS_SET_DENOISE, &disable);
    }

    speexPrepr.process(input, inputBuf);

    if (dtype == DenoiseType::Rnnoise) {
        rnnoiseDnsr.denoise(inputBuf, inputBuf);
    }

    lastDenoiseType = dtype;
}

int Audio::callback(const MonoFrame &input, Frame &output) noexcept {
    std::memset(&output, 0, sizeof(Frame));

    cbDenoise(input);

    if (lock.try_lock()) {
        if (!cbsToAdd.empty()) {
            callbacks.insert(callbacks.end(), cbsToAdd.begin(), cbsToAdd.end());
            cbsToAdd.clear();
        }
        lock.unlock();
    }

    int writers = 0;
    for (auto it = callbacks.begin(); it != callbacks.end();) {
        try {
            auto res = (*it)(inputBuf, outputBuf);
            if (res & CbFlagsWrite) {
                writers += 1;
                for (auto i : irange(FRAME_SIZE)) {
                    for (auto ch : irange(CHANNELS)) {
                        output.d[i][ch] += outputBuf.d[i][ch];
                    }
                }
            }
            if (res & CbFlagsStop) {
                it = callbacks.erase(it);
            } else {
                it++;
            }
        } catch (std::exception &ex) {
            it = callbacks.erase(it);
            CHAT_LOGE(boost::format("PortAudio callback catch exception: %1%") % ex.what());
        }
    }

    outputBuf = output; // safe last output;
    if (writers > 1) {
        compress(output);
    }
    if (callbacks.empty()) {
        return paComplete;
    } else {
        return paContinue;
    }
}
