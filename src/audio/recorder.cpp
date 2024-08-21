#include "audio.hpp"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include "portaudiocpp/SampleDataFormat.hxx"
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
    stream.open(params);
    st = State::Stopped;
}

Recorder::~Recorder() {
    st = State::Finalized;
    stream.close();
    cv.notify_all();
}

void Recorder::reconf() {
    bool isActive = stream.isActive();
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
    stream.open(params);
    if (isActive) {
        stream.start();
    }
}

void Recorder::start() {
    std::lock_guard g(stateMux);
    st = State::Active;
    stream.start();
    cv.notify_all();
}

void Recorder::stop() {
    std::lock_guard g(stateMux);
    st = State::Stopped;
    stream.stop();    
}

int Recorder::channels() const {
    return 1;
}

State Recorder::state() {
    return st;
}

void Recorder::read(Frame &frame) {
    frame.resize(FRAME_SIZE);
    stream.read(frame.data(), FRAME_SIZE);
    for (auto &dsp : dsps) {
        dsp->process(frame);
    }
}

void Recorder::waitActive() {
    std::unique_lock lk(stateMux);
    while (st == State::Stopped) {
        cv.wait(lk);
    }
}

void Recorder::lockState() {
    stateMux.lock();
}

void Recorder::unlockState() {
    stateMux.unlock();
}