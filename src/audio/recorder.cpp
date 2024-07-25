#include "audio.hpp"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include "portaudiocpp/SampleDataFormat.hxx"
#include <cassert>
#include <cstring>
#include <mutex>
#include <portaudio.h>
#include <rnnoise.h>

using namespace aud;

Recorder::Recorder() {
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
        FRAME_SIZE,
        paNoFlag
    );
    std::lock_guard<std::mutex> g(mux);
    stream.open(params);
    setState(State::Stopped);
}

Recorder::~Recorder() {
    {
        std::lock_guard<std::mutex> g(mux);
        setState(State::Finalized);
        stream.close();
    }
    cv.notify_all();
}

void Recorder::reconf() {
    stop();
    lockState();
    stream.close();
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
        FRAME_SIZE,
        paNoFlag
    );
    std::lock_guard<std::mutex> g(mux);
    stream.open(params);
    unlockState();
}

void Recorder::setState(State state) {
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
    if (stream.isActive()) {
        stream.stop();
    }
}

int Recorder::channels() const {
    return 1;
}

State Recorder::state() {
    std::lock_guard<std::mutex> g(mux);
    return st;
}

bool Recorder::read(Frame &frame) {
    {
        std::lock_guard<std::mutex> g(mux);
        frame.resize(FRAME_SIZE);
        stream.read(frame.data(), FRAME_SIZE);
    }
    for (auto &dsp : dsps) {
        dsp->process(frame);
    }
    return true;
}

void Recorder::waitActive() {
    if (state() == State::Stopped) {
        std::unique_lock<std::mutex> lk(cvMux);
        cv.wait(lk);
    }
}

void Recorder::lockState() {
    stateMux.lock();
}

void Recorder::unlockState() {
    stateMux.unlock();
}