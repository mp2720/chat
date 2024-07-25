#include "audio.hpp"
#include "portaudiocpp/BlockingStream.hxx"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>

using namespace aud;

Player::Player(std::shared_ptr<RawSource> src) : src(src) {
    assert(src);
    assert(0 < src->channels() && src->channels() <= getOutputDevice().maxOutputChannels());
    portaudio::DirectionSpecificStreamParameters outParams;
    outParams.setDevice(getOutputDevice());
    outParams.setNumChannels(src->channels());
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
    std::lock_guard<std::mutex> g(mux);
    stream.open(params);
    thrd = std::thread(&Player::tfunc, this);
}

Player::~Player() {
    src->stop();
    stream.close();
}

void Player::start() {
    src->start();
}

void Player::stop() {
    src->stop();
}

State Player::state() {
    return src->state();
}

void Player::reconf() {
    std::lock_guard<std::mutex> lg(mux);
    src->stop();
    src->lockState();
    stream.close();
    portaudio::DirectionSpecificStreamParameters outParams;
    outParams.setDevice(getOutputDevice());
    outParams.setNumChannels(src->channels());
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
    src->unlockState();
}

void Player::setVolume(float percentage) {
    volume = percentage / 100;
}

void Player::tfunc() {
    Frame buf;
    stream.start();
    while (1) {
        src->lockState();
        switch (src->state()) {
        case State::Active: {

            src->read(buf);
            src->unlockState();
            float vol = volume;
            if (vol != 1) {
                for (float &v : buf) {
                    v *= vol;
                }
            }
            try {
                stream.write(buf.data(), FRAME_SIZE);
            } catch (...) {
            }
        } break;

        case State::Stopped: {
            src->unlockState();
            {
                std::lock_guard<std::mutex> g(mux);
                if (!stream.isStopped()) {
                    stream.stop();
                }
            }
            src->waitActive();
            {
                std::lock_guard<std::mutex> g(mux);
                stream.start();
            }
        } break;

        case State::Finalized: {
            src->unlockState();
            endOfSourceCallback(*this);
            return;
        } break;
        }
    }
}