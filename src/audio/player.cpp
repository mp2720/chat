#include "audio.hpp"
#include "log.hpp"
#include "portaudiocpp/BlockingStream.hxx"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <portaudiocpp/Exception.hxx>
#include <string>
#include <thread>

using namespace aud;

Player::Player(std::shared_ptr<RawSource> src, shared_ptr<Output> out) {
    assert(src);
    assert(out);
    assert(0 < src->channels() && src->channels() <= getOutputDevice().maxOutputChannels());
    assert(src->channels() == out->channels());
    d = std::make_shared<PlayerData>();
    d->src = src;
    d->out = out;
    d->thrd = std::thread(&Player::playerThread, this);
    d->thrd.detach();
}

Player::~Player() {
    d->deleteFlag = true;
}

void Player::start() {
    d->src->start();
}

void Player::stop() {
    d->src->stop();
}

State Player::state() {
    return d->src->state();
}

void Player::setVolume(float percentage) {
    assert(percentage >= 0);
    d->volume = percentage / 100;
}

void Player::playerThread() {
    auto local_d = this->d;
    Frame buf;
    bool onActiveStart = true;
    while (1) {
        if (local_d->deleteFlag) {
            local_d->src->stop();
            return;
        }
        local_d->src->lockState();
        switch (local_d->src->state()) {
        case State::Active: {
            local_d->src->read(buf);
            local_d->src->unlockState();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            if (buf.empty())
                break;
            float vol = local_d->volume;
            if (vol != 1) {
                for (float &v : buf) {
                    v *= vol;
                }
            }
            if (onActiveStart) {
                onActiveStart = false;
                local_d->out->start();
            }
            local_d->out->write(buf);

        } break;

        case State::Stopped: {
            local_d->src->unlockState();
            if (!onActiveStart) {
                local_d->out->stop();
                onActiveStart = true;
            }
            local_d->src->waitActive();
        } break;

        case State::Finalized: {
            local_d->src->unlockState();
            if (endOfSourceCallback) {
                endOfSourceCallback();
            }
            return;
        } break;
        }
    }
}

PaOutput::PaOutput(int channels) : chans(channels) {
    assert(0 < channels && channels <= getOutputDevice().maxOutputChannels());
    portaudio::DirectionSpecificStreamParameters outParams;
    outParams.setDevice(getOutputDevice());
    outParams.setNumChannels(channels);
    outParams.setSampleFormat(portaudio::FLOAT32);
    outParams.setHostApiSpecificStreamInfo(nullptr);
    outParams.setSuggestedLatency(getOutputDevice().defaultHighOutputLatency());
    portaudio::StreamParameters params(
        portaudio::DirectionSpecificStreamParameters::null(),
        outParams,
        SAMPLE_RATE,
        FRAME_SIZE,
        paNoFlag
    );
    stream.open(params);
}

int PaOutput::channels() const {
    return chans;
}

void PaOutput::stop() {
    std::lock_guard<std::mutex> lg(mux);
    stream.stop();
}

void PaOutput::start() {
    std::lock_guard<std::mutex> lg(mux);
    stream.start();
}

void PaOutput::write(Frame &frame) {
    std::lock_guard<std::mutex> lg(mux);
    try {
        stream.write(frame.data(), FRAME_SIZE);
    } catch (portaudio::PaException &ex) {
        CHAT_LOGV(std::string("portaudio output: ") + ex.what());
    }
}

void PaOutput::reconf() {
    std::lock_guard<std::mutex> lg(mux);
    bool isActive = stream.isActive();
    if (isActive) {
        stream.close();
    }
    portaudio::DirectionSpecificStreamParameters outParams;
    outParams.setDevice(getOutputDevice());
    outParams.setNumChannels(chans);
    outParams.setSampleFormat(portaudio::FLOAT32);
    outParams.setHostApiSpecificStreamInfo(nullptr);
    outParams.setSuggestedLatency(getOutputDevice().defaultHighOutputLatency());
    portaudio::StreamParameters params(
        portaudio::DirectionSpecificStreamParameters::null(),
        outParams,
        SAMPLE_RATE,
        FRAME_SIZE,
        paNoFlag
    );
    stream.open(params);
    if (isActive) {
        stream.start();
    }
}
