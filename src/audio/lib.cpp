#include "audio.hpp"
#include "portaudiocpp/System.hxx"
#include <memory>
#include <mutex>
#include <portaudiocpp/PortAudioCpp.hxx>
#include <set>

using namespace aud;

shared_ptr<aud::Recorder> aud::mic = nullptr;

std::set<Reconfigurable *> reconfs;
std::mutex reconfsMux;

Device &aud::getOutputDevice() {
    return portaudio::System::instance().defaultOutputDevice();
}

Device &aud::getInputDevice() {
    return portaudio::System::instance().defaultInputDevice();
}

void aud::initialize() {
    portaudio::System::initialize();
    mic = std::make_shared<Recorder>();
}

void aud::terminate() {
    mic = nullptr;
    portaudio::System::terminate();
}

Reconfigurable::Reconfigurable() {
    std::lock_guard<std::mutex> lg(reconfsMux);
    reconfs.insert(this);
}

Reconfigurable::~Reconfigurable() {
    std::lock_guard<std::mutex> lg(reconfsMux);
    reconfs.erase(this);
}

void aud::reconfAll() {
    std::lock_guard<std::mutex> lg(reconfsMux);
    for (auto &v : reconfs) {
        v->reconf();
    }
}