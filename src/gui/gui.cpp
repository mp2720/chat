#include "gui.hpp"

using namespace chat::gui;

void Gui::init() {
    io.init();
}

void Gui::loopUntilExit() {
    while (io.shouldContinue()) {
        io.pollEvents();

        io.drawFrameStart();

        // UI

        io.drawFrameEnd();
    }
}

void Gui::terminate() {
    io.terminate();
}
