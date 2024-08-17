#include "audio.hpp"
#include "codec.hpp"
#include "log.hpp"
#include "opus.h"
#include "opus_defines.h"
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
    std::lock_guard lg(mux);
    mux.lock();
    if (buf.full()) {
        std::unique_lock ul(waitReadMux);
        mux.unlock();
        waitRead.wait(ul);
        mux.lock();
    }
    buf.push_back();
    buf.back().resize(pack.size());
    memcpy(buf.back().data(), pack.data(), pack.size());
    waitWrite.notify_one();
}

void NetBuf::pop(Frame &frame) {
    std::lock_guard lg(mux);
    if (buf.empty()) {
        std::unique_lock ul(waitWriteMux);
        mux.unlock();
        waitWrite.wait(ul);
        mux.lock();
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
}