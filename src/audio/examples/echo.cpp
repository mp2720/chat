#include "audio/audio.hpp"
#include "log.hpp"
#include <boost/stacktrace/stacktrace.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <portaudio.h>
#include <rnnoise.h>
#include <thread>

using namespace chat::aud;
using namespace chat;

int main() {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg) {
            return severity <= Logger::Severity::VERBOSE;
        }
    );

    global_logger.setOutput(&std::cerr);

    Audio::instance().addCallback([](const MonoFrame &in, Frame &out) {
        out = in;
        return CbFlagsWrite;
    });

    while (1) {
        Audio::instance().setDenoise(Audio::DenoiseType::None);
        puts("None");
        for (int i = 0; i < 5; i += 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("Load: %.2f%%\n", Pa_GetStreamCpuLoad(Audio::instance().getStream()) * 100);
        }
        Audio::instance().setDenoise(Audio::DenoiseType::Speex);
        puts("Speex");
        for (int i = 0; i < 5; i += 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("Load: %.2f%%\n", Pa_GetStreamCpuLoad(Audio::instance().getStream()) * 100);
        }
        Audio::instance().setDenoise(Audio::DenoiseType::Rnnoise);
        puts("Rnnoise");
        for (int i = 0; i < 5; i += 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("Load: %.2f%%\n", Pa_GetStreamCpuLoad(Audio::instance().getStream()) * 100);
        }
    }
}