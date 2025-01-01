#include "rnnoise.hpp"
#include "types.hpp"

void RnnoiseDenoiser::denoise(const MonoFrame &in, MonoFrame &out) {
    int rnnoise_frame_size = rnnoise_get_frame_size();
    assert(FRAME_SIZE >= (size_t)rnnoise_get_frame_size() && FRAME_SIZE % rnnoise_frame_size == 0);

    for (size_t i : irange(FRAME_SIZE)) {
        //+1 for a power of 2
        out.d[i] = in.d[i] * (INT16_MAX + 1);
    }

    for (size_t i : irange(FRAME_SIZE / rnnoise_frame_size)) {
        rnnoise_process_frame(
            state,
            out.d + rnnoise_frame_size * i,
            out.d + rnnoise_frame_size * i
        );
    }

    for (size_t i : irange(FRAME_SIZE)) {
        out.d[i] /= (INT16_MAX + 1);
    }
}