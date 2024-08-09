#include "audio.hpp"
#include <boost/exception/exception.hpp>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace aud {

class BufSrc : public RawSource {
  public:
    BufSrc(float buf[], size_t size, int channels = 1);
    void start() override;
    void stop() override;
    State state() override;
    void lockState() override;
    void unlockState() override;
    void waitActive() override; // if src stopped, requires a state check
    int channels() const override;
    void read(Frame &frame) override;

  private:
    State st = State::Active;
    float *buf;
    size_t size;
    const int chans;
    size_t played = 0;
    std::mutex mux;
    std::mutex stateMux;
    std::mutex cvMux;
    std::condition_variable cv;
};

} // namespace aud
