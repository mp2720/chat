#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
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

using TODO = int;

namespace aud {

using portaudio::Device;
using std::atomic;
using std::list;
using std::shared_ptr;
using std::unique_ptr;
using std::chrono::microseconds;

using Time = PaTime;

extern const int SAMPLE_RATE; // 48000 KHz

void initialize();
void terminate();

Device &getOutputDevice();
Device &getInputDevice();

class Source {
  public:
    enum class State {
        Active,
        Stopped,
        Finalized,
    };
    // if it returns a non-zero value, read should request the given number of frames
    // cannot be changed during execution
    virtual size_t frameSize() = 0;
    // writes n bytes to buf, where 0 <= n <= num, returns n
    // blocking function
    virtual size_t read(float frame[], size_t num) = 0;
    // returns the number of bytes ready for reading
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void term() = 0;
    virtual size_t available() = 0;

    std::mutex stateMux;
    virtual State state() = 0;

    virtual void waitStart() = 0; // if src stopped
    // when using n channels, 'read' must read n values by 1 'num'
    //'buf' must be at least n*num
    virtual int channels() const = 0;
    virtual ~Source() = default;
};

class ProgressSource : public Source {
  public:
    virtual Time getTime() = 0; // 0 - 100
    virtual Time getTotalTime() = 0;
    virtual bool setTime(Time val) = 0; // 'false' on fail
    virtual ~ProgressSource() = default;
};

class Player {
  public:
    Player(shared_ptr<Source> src);
    int channels();
    TODO getState();                  // for deletion on errors
    void setVolume(float percentage); // 0 - 100
    float getVolume();                // 0 - 100
    void stop();
    void start();
    void init(); // to change output device
    void term(); //
    std::function<void(Player &)> endOfSourceCallback;

  private:
    std::mutex mux;
    atomic<float> volume{1}; // 100%
    shared_ptr<Source> src;
    std::thread thrd;
    void tfunc();
    portaudio::BlockingStream stream;
};

class DSP {
  public:
    // is called automatically in a separate thread
    virtual void process(float frame[]) = 0;
    virtual ~DSP() = default;
};

class RnnoiseDSP : public DSP {
  public:
    RnnoiseDSP();
    ~RnnoiseDSP();
    void process(float frame[]) override;
    void on();
    void off();
    bool getState();

  private:
    atomic<bool> state{true};
    DenoiseState *handler;
};

class VolumeDSP : public DSP {
  public:
    void process(float frame[]) override;
    void set(float val); // 0 - 100 or more for amplification
    float get();

  private:
    atomic<float> val{1};
};

class Recorder : public Source {
  public:
    static size_t getFrameSize();
    void start() override;
    void stop() override;
    void init();          // to change input device
    void term() override; //
    size_t frameSize() override;
    size_t read(float frame[], size_t num) override;
    size_t available() override;
    Source::State state() override;
    void waitStart() override;
    int channels() const override;
    list<shared_ptr<DSP>> dsps;

  private:
    void setState(Source::State state);
    std::mutex mux;
    std::mutex cvMux;
    std::condition_variable cv;
    Source::State st;
    portaudio::BlockingStream stream;
};

extern shared_ptr<Recorder> mic;

} // namespace aud
