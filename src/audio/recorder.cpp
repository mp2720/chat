#include "audio.hpp"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include "portaudiocpp/SampleDataFormat.hxx"
#include <cassert>
#include <cstring>
#include <mutex>
#include <portaudio.h>
#include <rnnoise.h>

using namespace aud;

size_t Recorder::getFrameSize() {
    return (size_t)rnnoise_get_frame_size();
}

void Recorder::init() {
    portaudio::DirectionSpecificStreamParameters inParams;
    inParams.setDevice(getInputDevice());
    inParams.setNumChannels(1);
    inParams.setSampleFormat(portaudio::FLOAT32);
    inParams.setHostApiSpecificStreamInfo(nullptr);
    inParams.setSuggestedLatency(getInputDevice().defaultLowInputLatency());
    portaudio::StreamParameters params(
        inParams,
        portaudio::DirectionSpecificStreamParameters::null(),
        SAMPLE_RATE,
        (unsigned long)Recorder::getFrameSize(),
        paNoFlag
    );
    std::lock_guard<std::mutex> g(mux);
    stream.open(params);
    setState(State::Stopped);
}

void Recorder::setState(Source::State state) {
    std::lock_guard<std::mutex> g(cvMux);
    st = state;
}

void Recorder::start() {
    {
        std::lock_guard<std::mutex> g(mux);
        stream.start();
    }
    setState(State::Active);
    cv.notify_all();
}

void Recorder::stop() {
    setState(State::Stopped);
    std::lock_guard<std::mutex> g(mux);
    stream.stop();
}

void Recorder::term() {
    if (!stream.isStopped()) {
        stream.stop();
    }
    {
        std::lock_guard<std::mutex> g(mux);
        setState(State::Finalized);
        stream.close();
    }
    cv.notify_all();
}

int Recorder::channels() const {
    return 1;
}

Recorder::State Recorder::state() {
    std::lock_guard<std::mutex> g(mux);
    return st;
}

size_t Recorder::available() {
    std::lock_guard<std::mutex> g(mux);
    return stream.availableReadSize();
}

size_t Recorder::read(float frame[], size_t num) {
    assert(num == getFrameSize());
    {
        std::lock_guard<std::mutex> g(mux);
        stream.read(frame, num);
    }
    for (auto &dsp : dsps) {
        dsp->process(frame);
    }
    return num;
}

void Recorder::waitStart() {
    if (state() == State::Stopped) {
        std::unique_lock<std::mutex> lk(cvMux);
        cv.wait(lk);
    }
}

size_t Recorder::frameSize() {
    return Recorder::getFrameSize();
}