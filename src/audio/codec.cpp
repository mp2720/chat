#include "codec.hpp"
#include "audio.hpp"
#include "err.hpp"
#include "opus.h"
#include "opus_defines.h"
#include <cassert>
#include <cstddef>
#include <cstdint>

using namespace aud;

OpusException::OpusException(int error) throw() : err(error) {}

int OpusException::Error() const {
    return err;
}

const char *OpusException::ErrorText() const {
    return opus_strerror(err);
}

const char *OpusException::what() const throw() {
    return ErrorText();
}

bool OpusException::operator==(const OpusException &rhs) const {
    return err == rhs.err;
}

bool OpusException::operator!=(const OpusException &rhs) const {
    return err != rhs.err;
}

OpusEnc::OpusEnc(EncoderPreset ep, int channels) {
    int application;
    int bitrate;
    if (ep == EncoderPreset::Voise) {
        application = OPUS_APPLICATION_VOIP;
        bitrate = 24576; // bit/s
    } else {
        application = OPUS_APPLICATION_AUDIO;
        bitrate = 98304;
    }
    int err;
    enc = opus_encoder_create(aud::SAMPLE_RATE, channels, application, &err);
    if (err != OPUS_OK) {
        throw OpusException(err);
    }
    err = opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate));
    if (err < 0) {
        throw OpusException(err);
    }

    if (ep == EncoderPreset::Voise) {
        err = opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(1));
        if (err < 0) {
            throw OpusException(err);
        }
        err = opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(1));
        if (err < 0) {
            throw OpusException(err);
        }
    }
}

OpusEnc::~OpusEnc() {
    opus_encoder_destroy(enc);
}

void OpusEnc::encode(Frame &in, std::vector<uint8_t> &out, size_t max_size) {
    out.resize(max_size);
    int n_or_err =
        opus_encode_float(enc, in.data(), FRAME_SIZE, out.data(), max_size);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    out.resize(n_or_err);
}

void OpusEnc::setPacketLossPrec(int perc) {
    int err = opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(perc));
    if (err < 0) {
        throw OpusException(err);
    }
}

void OpusEncSrc::setPacketLossPrec(int perc) {
    enc.setPacketLossPrec(perc);
}

OpusEncSrc::OpusEncSrc(shared_ptr<RawSource> src, EncoderPreset ep)
    : enc(ep, src->channels()), src(src) {
    assert(src);
}

void OpusEncSrc::encode(std::vector<uint8_t> &block) {
    src->read(buf);
    enc.encode(buf, block);
}

void OpusEncSrc::lockState() {
    src->lockState();
}

void OpusEncSrc::unlockState() {
    src->unlockState();
}

void OpusEncSrc::start() {
    src->start();
}

void OpusEncSrc::stop() {
    src->stop();
}

State OpusEncSrc::state() {
    return src->state();
}

void OpusEncSrc::waitActive() {
    src->waitActive();
}

int OpusEncSrc::channels() const {
    int ch = src->channels();
    return ch;
}

OpusDecSrc::OpusDecSrc(shared_ptr<EncodedSource> src) : src(src) {
    CHAT_ASSERT(src);
    int err;
    dec = opus_decoder_create(SAMPLE_RATE, src->channels(), &err);
    if (err != OPUS_OK) {
        throw OpusException(err);
    }
}

OpusDecSrc::~OpusDecSrc() {
    opus_decoder_destroy(dec);
}

void OpusDecSrc::readLoss(Frame &frame) {
    frame.resize(FRAME_SIZE * src->channels());
    int n_or_err = opus_decode_float(dec, nullptr, 0, frame.data(), FRAME_SIZE, 0);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    CHAT_ASSERT(
        (size_t)n_or_err == FRAME_SIZE * src->channels() && "decoder must return full frame"
    );
}

void OpusDecSrc::readNormal(Frame &frame) {
    frame.resize(FRAME_SIZE * src->channels());
    int n_or_err = opus_decode_float(dec, buf.data(), buf.size(), frame.data(), FRAME_SIZE, 0);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    CHAT_ASSERT(
        (size_t)n_or_err == FRAME_SIZE * src->channels() && "decoder must return full frame"
    );
}

void OpusDecSrc::readFeh(Frame &frame) {
    frame.resize(FRAME_SIZE * src->channels());
    int n_or_err =
        opus_decode_float(dec, fehBuf.data(), fehBuf.size(), frame.data(), FRAME_SIZE, 1);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    CHAT_ASSERT(
        (size_t)n_or_err == FRAME_SIZE * src->channels() && "decoder must return full frame"
    );
}

void OpusDecSrc::readNoFeh(Frame &frame) {
    frame.resize(FRAME_SIZE * src->channels());
    int n_or_err =
        opus_decode_float(dec, fehBuf.data(), fehBuf.size(), frame.data(), FRAME_SIZE, 0);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    CHAT_ASSERT(
        (size_t)n_or_err == FRAME_SIZE * src->channels() && "decoder must return full frame"
    );
}

void OpusDecSrc::read(Frame &frame) {
    if (!fehFlag) {
        src->encode(buf);
        if (buf.empty()) {
            src->encode(fehBuf);
            if (fehBuf.empty()) {
                readLoss(frame);
            } else {
                readFeh(frame);
            }
            fehFlag = 1;
        } else {
            readNormal(frame);
        }
    } else {
        if (fehBuf.empty()) {
            src->encode(fehBuf);
            if (fehBuf.empty()) {
                readLoss(frame);
            } else {
                readFeh(frame);
            }
            fehFlag = 1;
        } else {
            readNoFeh(frame);
            fehFlag = 0;
        }
    }
}

void OpusDecSrc::lockState() {
    src->lockState();
}

void OpusDecSrc::unlockState() {
    src->unlockState();
}

void OpusDecSrc::start() {
    src->start();
}

void OpusDecSrc::stop() {
    src->stop();
}

State OpusDecSrc::state() {
    return src->state();
}

void OpusDecSrc::waitActive() {
    src->waitActive();
}

int OpusDecSrc::channels() const {
    int ch = src->channels();
    return ch;
}
