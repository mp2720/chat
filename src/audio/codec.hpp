#pragma once

#include "audio.hpp"
#include <exception>
#include <opus/opus.h>

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

class EncodedSource : public Source {
  public:
    // block must be at least MAX_ENCODER_BLOCK_SIZE
    // returns the size of the recorded block
    virtual size_t encode(unsigned char *block) = 0;
    virtual ~EncodedSource() = default;
};

class Encoder : public EncodedSource {
  public:
    Encoder(shared_ptr<RawSource> src, EncoderPreset ep);
    ~Encoder();
    void lockState() override;
    void unlockState() override;
    void start() override;
    void stop() override;
    State state() override;
    void waitActive() override;
    int channels() const override;
    size_t encode(unsigned char *block) override;
  private:
    OpusEncoder *enc;
    shared_ptr<RawSource> src;
    unique_ptr<float[]> buf;
};

class Decoder : public RawSource {
  public:
    Decoder(shared_ptr<EncodedSource> src);
    ~Decoder();
    void lockState() override;
    void unlockState() override;
    void start() override;
    void stop() override;
    bool read(float frame[]) override;
    State state() override;
    void waitActive() override;
    int channels() const override;
    std::mutex stateMux;
  private:
    OpusDecoder *dec;
    shared_ptr<EncodedSource> src;
    unique_ptr<unsigned char[]> buf;
};

} // namespace aud