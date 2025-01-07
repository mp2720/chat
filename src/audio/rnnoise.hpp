#pragma once

#include "defs.hpp"
#include "rnnoise.h"
#include <boost/range/irange.hpp>
#include <cassert>

using namespace chat::aud;
using namespace chat;

class RnnoiseDenoiser {
  public:
    RnnoiseDenoiser(RNNModel *model = nullptr);
    ~RnnoiseDenoiser();
    void denoise(const MonoFrame &in, MonoFrame &out);

  private:
    DenoiseState *state;
    int rnnoise_frame_size;
};