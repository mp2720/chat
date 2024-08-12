#pragma once

#include "audio.hpp"
#include <cstdint>
#include <exception>
#include <opus/opus.h>
#include <vector>

namespace aud {

inline constexpr size_t MAX_ENCODER_BLOCK_SIZE = 1024;

class OpusException : std::exception {
  public:
    OpusException(int error) throw();
    const char *what() const throw();
    int Error() const;
    const char *ErrorText() const;
    bool operator==(const OpusException &rhs) const;
    bool operator!=(const OpusException &rhs) const;

  private:
    int err;
};

enum class EncoderPreset {
    Voise,
    Sounds,
};

class OpusEnc {
  public:
    OpusEnc(EncoderPreset ep, int channels);
    ~OpusEnc();
    void setPacketLossPrec(int perc);
    void encode(Frame &in, std::vector<uint8_t> &out, size_t max_size = MAX_ENCODER_BLOCK_SIZE);

  private:
    OpusEncoder *enc;
};

class EncodedSource : public Source {
  public:
    // empty block means packet loss
    virtual void encode(std::vector<uint8_t> &block) = 0;
    virtual void setPacketLossPrec(int perc) = 0;
    virtual ~EncodedSource() = default;
};

class OpusEncSrc : public EncodedSource {
  public:
    OpusEncSrc(shared_ptr<RawSource> src, EncoderPreset ep);
    void lockState() override;
    void unlockState() override;
    void start() override;
    void stop() override;
    State state() override;
    void waitActive() override;
    int channels() const override;
    void setPacketLossPrec(int perc) override;
    void encode(std::vector<uint8_t> &block) override;

  private:
    OpusEnc enc;
    shared_ptr<RawSource> src;
    Frame buf;
};

class OpusDecSrc : public RawSource {
  public:
    OpusDecSrc(shared_ptr<EncodedSource> src);
    ~OpusDecSrc();
    void lockState() override;
    void unlockState() override;
    void start() override;
    void stop() override;
    void read(Frame &frame) override;
    State state() override;
    void waitActive() override;
    int channels() const override;
    std::mutex stateMux;

  private:
    void readLoss(Frame &frame);
    void readFeh(Frame &frame);
    void readNoFeh(Frame &frame);
    void readNormal(Frame &frame);

    bool fehFlag = 0;
    std::vector<uint8_t> fehBuf;
    OpusDecoder *dec;
    shared_ptr<EncodedSource> src;
    std::vector<uint8_t> buf;
};

} // namespace aud