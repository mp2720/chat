#include "audio/audio.hpp"
#include "sources.hpp"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <mutex>

using namespace aud;

BufSrc::BufSrc(float buf[], size_t size, int channels) : buf(buf), size(size), chans(channels) {
    assert(size % FRAME_SIZE == 0);
}

void BufSrc::start() {
    std::lock_guard<std::mutex> lg(mux);
    std::lock_guard<std::mutex> lg1(stateMux);
    st = State::Active;
    cv.notify_all();
}

void BufSrc::stop() {
    std::lock_guard<std::mutex> lg(mux);
    std::lock_guard<std::mutex> lg1(stateMux);
    st = State::Stopped;
}

State BufSrc::state() {
    std::lock_guard<std::mutex> lg(mux);
    if (played >= size) {
        std::lock_guard<std::mutex> lg(stateMux);
        st = State::Finalized;
    }
    return st;
}

void BufSrc::lockState() {
    stateMux.lock();
}

void BufSrc::unlockState() {
    stateMux.unlock();
}

void BufSrc::waitActive() {
    if (state() == State::Stopped) {
        std::unique_lock<std::mutex> lk(cvMux);
        cv.wait(lk);
    }
}

int BufSrc::channels() const {
    return chans;
}

void BufSrc::read(Frame &frame) {
    std::lock_guard<std::mutex> lg(mux);
    frame.resize(FRAME_SIZE * chans);
    if (played < size) {
        std::memcpy(frame.data(), buf + played, FRAME_SIZE * chans * sizeof(float));
        played += FRAME_SIZE * chans;
    } else {
        memset(frame.data(), 0, sizeof(float) * FRAME_SIZE * chans);
    }
}
