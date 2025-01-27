#include "audio.hpp"
#include "audio/defs.hpp"
#include "log.hpp"
#include "types.hpp"
#include <boost/range/irange.hpp>
#include <cstdint>

using namespace chat::aud;
using namespace chat;

static void compress(const int32_t sum[FRAME_SIZE][CHANNELS], Frame &result) {
    int32_t maxVal = 0;
    for (auto i : irange(FRAME_SIZE)) {
        for (auto ch : irange(CHANNELS)) {
            maxVal = std::max(maxVal, std::abs(sum[i][ch]));
        }
    }
    if (maxVal > INT16_MAX) {
        float k = static_cast<float>(INT16_MAX) / static_cast<float>(maxVal);
        for (auto i : irange(FRAME_SIZE)) {
            for (auto ch : irange(CHANNELS)) {
                result.d[i][ch] = static_cast<int16_t>(static_cast<float>(sum[i][ch]) * k);
            }
        }
    }
}

void Audio::cbDenoise(MonoFrame &frame) {
    auto dtype = denoiseType.load();

    if (dtype == DenoiseType::Speex && lastDenoiseType != DenoiseType::Speex) {
        int enable = 1;
        speexPrepr.control(SPEEX_PREPROCESS_SET_DENOISE, &enable);

    } else if (dtype != DenoiseType::Speex && lastDenoiseType == DenoiseType::Speex) {
        int disable = 0;
        speexPrepr.control(SPEEX_PREPROCESS_SET_DENOISE, &disable);
    }

    speexPrepr.process(frame);

    if (dtype == DenoiseType::Rnnoise) {
        rnnoiseDnsr.denoise(frame, frame);
    }

    lastDenoiseType = dtype;
}

int Audio::callback(const MonoFrame &input, Frame &output) noexcept {
    inputBuf = input;

    cbDenoise(inputBuf);

    if (lock.try_lock()) {
        if (!cbsToAdd.empty()) {
            callbacks.insert(callbacks.end(), cbsToAdd.begin(), cbsToAdd.end());
            cbsToAdd.clear();
        }
        lock.unlock();
    }

    int32_t sum_buffer[FRAME_SIZE][CHANNELS];
    std::memset(sum_buffer, 0, sizeof(sum_buffer));

    int writers = 0;
    for (auto it = callbacks.begin(); it != callbacks.end();) {
        try {
            auto res = (*it)(inputBuf, output);
            if (res & CbFlagsWrite) {
                writers += 1;
                for (auto i : irange(FRAME_SIZE)) {
                    for (auto ch : irange(CHANNELS)) {
                        sum_buffer[i][ch] += output.d[i][ch];
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

    if (writers > 1) {
        compress(sum_buffer, output);
    }
    if (callbacks.empty()) {
        return paComplete;
    } else {
        return paContinue;
    }
}
