#pragma once

#include <boost/range/irange.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <portaudio.h>

namespace chat {
namespace aud {

constexpr int SAMPLE_RATE = 48000;
constexpr size_t FRAME_SIZE = 960;
constexpr int CHANNELS = 2;
constexpr size_t MAX_ENCODER_BLOCK_SIZE = 128;

struct MonoFrame;

struct Frame {
    Frame() = default;
    Frame(const MonoFrame &mframe);
    int16_t d[FRAME_SIZE][CHANNELS];
};

struct MonoFrame {
    MonoFrame() = default;
    MonoFrame(const Frame &frame);
    int16_t d[FRAME_SIZE];
};

using CbFlags = uint32_t;
constexpr CbFlags CbFlagsNone = 0x0;
constexpr CbFlags CbFlagsStop = 0x1;
constexpr CbFlags CbFlagsWrite = 0x2;
using Callback = std::function<CbFlags(const MonoFrame &input, Frame &output)>;

class Exception : public std::exception {
  public:
    Exception(PaError code) : err(code) {}
    virtual const char *what() const noexcept override {
        return Pa_GetErrorText(err);
    }
    PaError Error() const noexcept {
        return err;
    }

  private:
    PaError err;
};

} // namespace aud
} // namespace chat