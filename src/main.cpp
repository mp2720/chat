#include "gui/renderer.hpp"
#include "gui/win.hpp"
#include "log.hpp"
#include <iostream>

using namespace chat;
using namespace chat::gui;

int main(int, char **) {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg) {
            return severity <= Logger::VERBOSE;
        }
    );
    global_logger.setOutput(&std::cerr);

    auto win_sys = makeGlfwWindowSystem();
    RendererConfig renderer_conf{true, true};
    auto main_win = win_sys->createWindow(renderer_conf);

    win_sys->runLoop();
}
