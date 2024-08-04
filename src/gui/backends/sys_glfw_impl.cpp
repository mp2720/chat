#include "sys_glfw_impl.hpp"

#include "log.hpp"
#include <GLFW/glfw3.h>
#include <boost/format.hpp>

using namespace chat::gui::backends;

using boost::format, boost::str;

void GlfwSystemWindow::handleGlfwError() {
    const char *description;
    int code = glfwGetError(&description);

    throw IoException(str(format("GLFW error %1% %2%") % code % description));
}

void GlfwSystemWindow::onGlfwErrorCallback(int error, const char *description) noexcept {
    CHAT_LOGE(format("GLFW error %1% %2%") % error % description);
}

void GlfwSystemWindow::onWindowRefresh(GLFWwindow *window) noexcept {
    auto this_ = getThis(window);
    this_->refresh_required = true;
}

void GlfwSystemWindow::onWindowResize(GLFWwindow *window, int width, int height) noexcept {
    auto this_ = getThis(window);
    this_->frame_buffer_size = {width, height};
    this_->resize_required = true;
    this_->refresh_required = true;
}

GlfwSystemWindow::GlfwSystemWindow() {
    glfwSetErrorCallback(onGlfwErrorCallback);

    // This specific title is needed to configure WMs (like i3) to move window to another workspace.
    // i3 need this title to be set during the window creation. The actual title can be set later
    // using setTitle().
    glfw_window = glfwCreateWindow(1280, 720, "mp2720/chat", nullptr, nullptr);
    if (glfw_window == nullptr)
        handleGlfwError();

    glfwMakeContextCurrent(glfw_window);

    glfwSetWindowUserPointer(glfw_window, this);
    glfwSetWindowRefreshCallback(glfw_window, onWindowRefresh);
    glfwSetFramebufferSizeCallback(glfw_window, onWindowResize);

    // initial values.
    glfwGetFramebufferSize(glfw_window, &frame_buffer_size.x, &frame_buffer_size.y);

    // vsync
    glfwSwapInterval(1);
}
