#include "rnnoise.hpp"
#include "audio/defs.hpp"
#include "types.hpp"
#include <cmath>
#include <cstdint>

RnnoiseDenoiser::RnnoiseDenoiser(RNNModel *model) {
    state = rnnoise_create(model);
    assert(state != nullptr);
    rnnoise_frame_size = rnnoise_get_frame_size();
    assert(FRAME_SIZE >= (size_t)rnnoise_get_frame_size() && FRAME_SIZE % rnnoise_frame_size == 0);
}

RnnoiseDenoiser::~RnnoiseDenoiser() {
    rnnoise_destroy(state);
}

void RnnoiseDenoiser::denoise(const MonoFrame &in, MonoFrame &out) {
    float buf[FRAME_SIZE];

    for (size_t i : irange(FRAME_SIZE)) {
        buf[i] = static_cast<float>(in.d[i]);
    }

    for (size_t i : irange(FRAME_SIZE / rnnoise_frame_size)) {
        rnnoise_process_frame(state, buf + rnnoise_frame_size * i, buf + rnnoise_frame_size * i);
    }

    for (size_t i : irange(FRAME_SIZE)) {
        out.d[i] = static_cast<int16_t>(buf[i]);
    }
}