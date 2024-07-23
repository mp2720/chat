#include "audio.hpp"
#include "portaudiocpp/System.hxx"
#include <memory>
#include <portaudiocpp/PortAudioCpp.hxx>

const int aud::SAMPLE_RATE = 48000;

std::shared_ptr<aud::Recorder> aud::mic;

aud::Device &aud::getOutputDevice() {
    return portaudio::System::instance().defaultOutputDevice();
}

aud::Device &aud::getInputDevice() {
    return portaudio::System::instance().defaultInputDevice();
}

void aud::initialize() {
    portaudio::System::initialize();
    aud::mic = std::make_shared<aud::Recorder>();
    aud::mic->init();
}

void aud::terminate() {
    aud::mic->term();
    portaudio::System::terminate();
}