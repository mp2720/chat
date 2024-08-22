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
    d->waitThreadStart.lock();
    std::thread(&Player::playerThread, this).detach();
    d->waitThreadStart.lock(); //wait unlock from playerThread()
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
    auto d = this->d;
    Frame buf;
    bool onActiveStart = true;
    d->waitThreadStart.unlock();
    while (1) {
        if (d->deleteFlag) {
            d->src->stop();
            return;
        }
        d->src->lockState();
        switch (d->src->state()) {
        case State::Active: {
            d->src->read(buf);
            d->src->unlockState();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            if (buf.empty())
                break;
            float vol = d->volume;
            if (vol != 1) {
                for (float &v : buf) {
                    v *= vol;
                }
            }
            if (onActiveStart) {
                onActiveStart = false;
                d->out->start();
            }
            d->out->write(buf);

        } break;

        case State::Stopped: {
            d->src->unlockState();
            if (!onActiveStart) {
                d->out->stop();
                onActiveStart = true;
            }
            d->src->waitActive();
        } break;

        case State::Finalized: {
            d->src->unlockState();
            d->out->stop();
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
