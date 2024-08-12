#include "audio.hpp"
#include "codec.hpp"
#include "opus.h"
#include "opus_defines.h"
#include <cmath>
#include <cstdio>
#include <mutex>
#include "log.hpp"

using namespace aud;

NetBuf::NetBuf(size_t depth, int channels) : depth(depth), chans(channels), buf(depth * 3) {
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

void NetBuf::push(std::vector<uint8_t> data) {
    if (buf.full()) {
        std::unique_lock ul(waitReadMux);
        waitRead.wait(ul);
    }
    std::lock_guard lg(mux);
    buf.push_back(std::move(data));
    waitWrite.notify_one();
}

void NetBuf::pop(Frame &frame) {
    frame.resize(FRAME_SIZE * chans);
    if (buf.empty()) {
        std::unique_lock ul(waitWriteMux);
        waitWrite.wait(ul);
    }
    std::lock_guard lg(mux);
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
    if (buf.size() > depth) {
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