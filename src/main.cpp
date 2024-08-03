#include "gui/backends/graphics_gl_impl.hpp"
#include "gui/backends/sys_glfw_impl.hpp"
#include "gui/window.hpp"
#include "log.hpp"
#include "ptr.hpp"
#include <iostream>

using namespace chat;
using namespace chat::gui;

int main(int, char **) {
    global_logger.setFilter(
        [](Logger::Severity severity, const char *file, long line, const std::string &msg) {
            return true;
            return severity <= Logger::VERBOSE;
        }
    );
    global_logger.setOutput(&std::cerr);

    glfwInit();
    {
        unique_ptr<SystemWindow> sys_win = std::make_unique<GlfwSystemWindow>();
        sys_win->switchContext();

        unique_ptr<Renderer> renderer = std::make_unique<GlRenderer>(Color{1, 1, 0, 1}, true);

        Window window(std::move(sys_win), std::move(renderer));

        while (window.canContinue()) {
            window.update();

            if (window.needRedraw()) {
                glfwPollEvents();
                continue;
            }

            glfwWaitEvents();
        }
    }
    glfwTerminate();
}
