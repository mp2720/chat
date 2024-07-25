#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <portaudio.h>
#include <portaudiocpp/BlockingStream.hxx>
#include <portaudiocpp/Device.hxx>
#include <portaudiocpp/PortAudioCpp.hxx>
#include <rnnoise.h>
#include <thread>
#include <vector>

namespace aud {

inline constexpr int SAMPLE_RATE = 48000; // 48000 KHz
inline constexpr size_t FRAME_SIZE = 480; // 480

using portaudio::Device;
using std::atomic;
using std::list;
using std::shared_ptr;
using std::unique_ptr;

using portaudio::Device;

using Time = PaTime;

using Frame = std::vector<float>;

void initialize();
void terminate();
Device &getOutputDevice();
Device &getInputDevice();
void reconfAll();

enum class State {
    Active,
    Stopped,
    Finalized,
};

class Reconfigurable {
  public:
    Reconfigurable();
    ~Reconfigurable();
    // to change the input-output device on-the-fly
    virtual void reconf() = 0;
};

class Controllable {
  public:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual State state() = 0;
    virtual ~Controllable() = default;
};

class Source : public Controllable {
  public:
    virtual void lockState() = 0;
    virtual void unlockState() = 0;
    virtual void waitActive() = 0; // if src stopped, requires a state check
    virtual int channels() const = 0;
    virtual ~Source() = default;
};

class RawSource : public Source {
  public:
    // frame must be at least FRAME_SIZE * channels
    virtual bool read(Frame &frame) = 0;
    virtual ~RawSource() = default;
};

class Player : public Controllable, public Reconfigurable {
  public:
    Player(shared_ptr<RawSource> src);
    ~Player();
    void setVolume(float percentage); // 0 - 100
    float getVolume();                // 0 - 100
    void start() override;
    void stop() override;
    State state() override;
    void reconf() override;
    std::function<void(Player &)> endOfSourceCallback;

  private:
    std::mutex mux;
    atomic<float> volume = 1; // 100%
    shared_ptr<RawSource> src;
    std::thread thrd;
    void tfunc();
    portaudio::BlockingStream stream;
};

class DSP {
  public:
    // is called automatically in a separate thread
    virtual void process(Frame &frame) = 0;
    virtual ~DSP() = default;
};

class RnnoiseDSP : public DSP {
  public:
    RnnoiseDSP();
    ~RnnoiseDSP();
    void process(Frame &frame) override;
    void on();
    void off();
    bool getState();

  private:
    atomic<bool> state{true};
    DenoiseState *handler;
};

class VolumeDSP : public DSP {
  public:
    void process(Frame &frame) override;
    void set(float val); // 0 - 100 or more for amplification
    float get();

  private:
    atomic<float> val{1};
};

class Recorder : public RawSource, public Reconfigurable {
  public:
    Recorder();
    ~Recorder();
    void lockState() override;
    void unlockState() override;
    void start() override;
    void stop() override;
    bool read(Frame &frame) override;
    State state() override;
    void waitActive() override;
    int channels() const override;
    void reconf() override;
    list<shared_ptr<DSP>> dsps;

  private:
    void setState(State state);
    std::mutex mux;
    std::mutex cvMux;
    std::mutex stateMux;
    std::condition_variable cv;
    State st;
    portaudio::BlockingStream stream;
};

extern shared_ptr<Recorder> mic;

} // namespace aud