#include "audio.hpp"
#include "err.hpp"
#include "log.hpp"
#include "portaudiocpp/BlockingStream.hxx"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <portaudiocpp/Exception.hxx>
#include <string>
#include <thread>

using namespace aud;

Player::Player(std::shared_ptr<RawSource> src, shared_ptr<Output> out) {
    CHAT_ASSERT(src);
    CHAT_ASSERT(out);
    CHAT_ASSERT(0 < src->channels() && src->channels() <= getOutputDevice().maxOutputChannels());
    CHAT_ASSERT(src->channels() == out->channels());
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
    d->volume = percentage / 100;
}

void Player::playerThread() {
    auto local_d = this->d;
    Frame buf;
    local_d->out->start();
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
            if (buf.empty())
                break;
            float vol = local_d->volume;
            if (vol != 1) {
                for (float &v : buf) {
                    v *= vol;
                }
            }
            local_d->out->write(buf);

        } break;

        case State::Stopped: {
            local_d->src->unlockState();
            {
                local_d->out->stop();
            }
            local_d->src->waitActive();
            {
                local_d->out->start();
            }
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
    CHAT_ASSERT(0 < channels && channels <= getOutputDevice().maxOutputChannels());
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
    if (!stream.isStopped()) {
        stream.stop();
    }
}

void PaOutput::start() {
    std::lock_guard<std::mutex> lg(mux);
    if (!stream.isActive()) {
        stream.start();
    }
}

void PaOutput::write(Frame &frame) {
    std::lock_guard<std::mutex> lg(mux);
    try {
        stream.write(frame.data(), FRAME_SIZE);
    } catch (portaudio::PaException &ex) {
        CHAT_LOGW(std::string("portaudio output:") + ex.what());
    }
}

void PaOutput::reconf() {
    std::lock_guard<std::mutex> lg(mux);
    bool isActive = stream.isActive();
    stream.close();
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