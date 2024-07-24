#include "codec.hpp"
#include "audio.hpp"
#include "opus.h"
#include "opus_defines.h"
#include <cassert>
#include <cstddef>
#include <memory>

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

Encoder::Encoder(shared_ptr<RawSource> src, EncoderPreset ep) : src(src){
    assert(src);
    int application;
    int bitrate;
    int channels = src->channels();
    if (ep == EncoderPreset::Voise) {
        application = OPUS_APPLICATION_VOIP;
        bitrate = 32768; //bit/s per channel
    } else {
        application = OPUS_APPLICATION_AUDIO;
        bitrate = 65536;
    }
    int err;
    enc = opus_encoder_create(aud::SAMPLE_RATE, channels, application, &err);
    if (err != OPUS_OK) {
        throw OpusException(err);
    }
    err = opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate));
    if(err < 0) {
        throw OpusException(err);
    }

    buf = std::make_unique<float[]>(FRAME_SIZE * channels);
}

Encoder::~Encoder() {
    opus_encoder_destroy(enc);
}  

size_t Encoder::encode(unsigned char *block) {
    src->read(buf.get());
    int n_or_err = opus_encode_float(enc, buf.get(), FRAME_SIZE, block, MAX_ENCODER_BLOCK_SIZE);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    return n_or_err;
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
    assert(src);
    int err;
    dec = opus_decoder_create(SAMPLE_RATE, src->channels(), &err);
    if (err != OPUS_OK) {
        throw OpusException(err);
    }
    buf = std::make_unique<unsigned char[]>(MAX_ENCODER_BLOCK_SIZE);
}

Decoder::~Decoder() {
    opus_decoder_destroy(dec);
}

bool Decoder::read(float frame[]) {
    size_t n = src->encode(buf.get());
    int n_or_err = opus_decode_float(dec, buf.get(), n, frame, FRAME_SIZE, false);
    if (n_or_err < 0) {
        throw OpusException(n_or_err);
    }
    assert(n_or_err == FRAME_SIZE);
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

