#include "audio.hpp"
#include "codec.hpp"
#include "log.hpp"
#include "opus.h"
#include "opus_defines.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>

using namespace aud;

NetBuf::NetBuf(size_t depth, int channels) : depth(depth), chans(channels), buf(depth * 2) {
    int err;
    dec = opus_decoder_create(aud::SAMPLE_RATE, channels, &err);
    if (err < 0) {
        CHAT_LOGE(opus_strerror(err));
        throw OpusException(err);
    }
}

NetBuf::~NetBuf() {
    opus_decoder_destroy(dec);
}

void NetBuf::push(boost::span<uint8_t> pack) {
    assert(pack.size() <= MAX_ENCODER_BLOCK_SIZE);
    std::unique_lock lg(mux);
    while (buf.full()) {
        waitRead.wait(lg);
    }
    buf.push_back();
    std::copy(pack.begin(), pack.end(), std::back_inserter(buf.back()));
    waitWrite.notify_one();
}

void NetBuf::read(Frame &frame) {
    std::unique_lock lg(mux);

    while (buf.empty()) {
        waitWrite.wait(lg);
    }

    frame.resize(FRAME_SIZE * chans);
    int err = opus_decode_float(
        dec,
        buf.front().data(),
        (int)buf.front().size(),
        frame.data(),
        (int)frame.size(),
        0
    );
    if (err < 0) {
        throw OpusException(err);
    }
    buf.pop_front();

    if (buf.size() + 1 > depth) {
        frameBuf.resize(FRAME_SIZE * chans);
        int err = opus_decode_float(
            dec,
            buf.front().data(),
            (int)buf.front().size(),
            frameBuf.data(),
            (int)frameBuf.size(),
            0
        );
        if (err < 0) {
            throw OpusException(err);
        }
        for (size_t i = 0; i < frame.size(); i++) {
            frame[i] = (frame[i] + frameBuf[i]) / 2;
        }
        buf.pop_front();
    }
    waitRead.notify_one();
}