#include "audio.hpp"
#include "portaudiocpp/System.hxx"
#include <memory>
#include <portaudiocpp/PortAudioCpp.hxx>
#include <set>


using namespace aud;

shared_ptr<aud::Recorder> aud::mic = nullptr;

std::set<Reconfigurable*> reconfs;

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
    reconfs.insert(this);
}

Reconfigurable::~Reconfigurable() {
    reconfs.erase(this);
}

void aud::reconfAll() {
    for (auto &v : reconfs) {
        v->reconf();
    }
}