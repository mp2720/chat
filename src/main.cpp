#include "gui/gui.hpp"
#include "log.hpp"
#include <boost/format/format_fwd.hpp>
#include <iostream>

using namespace chat;

int main(int, char **) {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg){
            return severity <= Logger::Severity::VERBOSE;
        }
    );
    global_logger.setOutput(&std::cerr);
    gui::Gui gui;
    gui.init();
    gui.loopUntilExit();
    gui.terminate();
}
