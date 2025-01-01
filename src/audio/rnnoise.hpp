#pragma once

#include "defs.hpp"
#include "rnnoise.h"
#include <boost/range/irange.hpp>
#include <cassert>

using namespace chat::aud;
using namespace chat;

class RnnoiseDenoiser {
  public:
    RnnoiseDenoiser(RNNModel *model = nullptr) {
        state = rnnoise_create(model);
        assert(state != nullptr);
    }

    ~RnnoiseDenoiser() {
        rnnoise_destroy(state);
    }

    void denoise(const MonoFrame &in, MonoFrame &out);

  private:
    DenoiseState *state;
};