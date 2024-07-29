#include "codec.hpp"
#include "audio.hpp"
#include "err.hpp"
#include "opus.h"
#include "opus_defines.h"
#include <cassert>
#include <cstddef>

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

Encoder::Encoder(shared_ptr<RawSource> src, EncoderPreset ep) : src(src) {
    CHAT_ASSERT(src);
    int application;
    int bitrate;
    int channels = src->channels();
    if (ep == EncoderPreset::Voise) {
        application = OPUS_APPLICATION_VOIP;
        bitrate = 32768; // bit/s
    } else {
        application = OPUS_APPLICATION_AUDIO;
        bitrate = 131072;
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
}

Encoder::~Encoder() {
    opus_encoder_destroy(enc);
}

void Encoder::encode(std::vector<uint8_t> &block) {
    src->read(buf);
    block.resize(MAX_ENCODER_BLOCK_SIZE);
    int n_or_err =
        opus_encode_float(enc, buf.data(), FRAME_SIZE, block.data(), MAX_ENCODER_BLOCK_SIZE);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    block.resize(n_or_err);
}

void Encoder::lockState() {
    src->lockState();
}

void Encoder::unlockState() {
    src->unlockState();
}

void Encoder::start() {
    src->start();
}

void Encoder::stop() {
    src->stop();
}

State Encoder::state() {
    return src->state();
}

void Encoder::waitActive() {
    src->waitActive();
}

int Encoder::channels() const {
    int ch = src->channels();
    return ch;
}

Decoder::Decoder(shared_ptr<EncodedSource> src) : src(src) {
    CHAT_ASSERT(src);
    int err;
    dec = opus_decoder_create(SAMPLE_RATE, src->channels(), &err);
    if (err != OPUS_OK) {
        throw OpusException(err);
    }
}

Decoder::~Decoder() {
    opus_decoder_destroy(dec);
}

bool Decoder::read(Frame &frame) {
    src->encode(buf);
    frame.resize(FRAME_SIZE * src->channels());
    int n_or_err = opus_decode_float(dec, buf.data(), buf.size(), frame.data(), FRAME_SIZE, false);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    CHAT_ASSERT((size_t)n_or_err == FRAME_SIZE * src->channels() && "decoder must return full frame");
    return true;
}

void Decoder::lockState() {
    src->lockState();
}

void Decoder::unlockState() {
    src->unlockState();
}

void Decoder::start() {
    src->start();
}

void Decoder::stop() {
    src->stop();
}

State Decoder::state() {
    return src->state();
}

void Decoder::waitActive() {
    src->waitActive();
}

int Decoder::channels() const {
    int ch = src->channels();
    return ch;
}
