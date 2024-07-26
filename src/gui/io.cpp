#include "io.hpp"

#include "gui.hpp"
#include "log.hpp"
#include <GLFW/glfw3.h>
#include <boost/format.hpp>

using namespace chat::gui;

using boost::format, boost::str;

static void onGlfwErrorCallback(int error, const char *description) {
    CHAT_LOGE(format("GLFW error %1% %2%") % error % description);
}

static void handleGlfwError() {
    const char *description;
    int code = glfwGetError(&description);

    throw Exception(str(format("%1% %2%") % code % description));
}

void Io::init() {
    glfwSetErrorCallback(onGlfwErrorCallback);
    if (!glfwInit())
        handleGlfwError();

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(1280, 720, "chat", nullptr, nullptr);
    if (window == nullptr)
        handleGlfwError();

    glfwMakeContextCurrent(window);

    // vsync
    glfwSwapInterval(1);
}

bool Io::shouldContinue() {
    return !glfwWindowShouldClose(window);
}

void Io::pollEvents() {
    glfwPollEvents();
}

void Io::drawFrameStart() {
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(
        CLEAR_COLOR.x * CLEAR_COLOR.w,
        CLEAR_COLOR.y * CLEAR_COLOR.w,
        CLEAR_COLOR.z * CLEAR_COLOR.w,
        CLEAR_COLOR.w
    );
    glClear(GL_COLOR_BUFFER_BIT);
}

void Io::drawFrameEnd() {
    glfwSwapBuffers(window);
}

void Io::terminate() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
