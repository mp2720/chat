#pragma once

#include "../gl_include.h"
#include "sys.hpp"

namespace chat::gui::backends {

class GlfwSystemWindow : public SystemWindow {
  private:
    GLFWwindow *glfw_window;
    bool refresh_required = true;
    bool resize_required = true;
    Vec2I frame_buffer_size{};

    static GlfwSystemWindow *getThis(GLFWwindow *win) noexcept {
        auto *this_ = static_cast<GlfwSystemWindow *>(glfwGetWindowUserPointer(win));
        assert(
            this_ != nullptr && "GLFW user pointer must be set to GlfwSystemWindow object instance"
        );
        return this_;
    }

    static void handleGlfwError();

    static void onGlfwErrorCallback(int error, const char *description) noexcept;

    static void onWindowRefresh(GLFWwindow *window) noexcept;

    static void onWindowResize(GLFWwindow *window, int width, int height) noexcept;

  public:
    GlfwSystemWindow();

    void switchContext() {
        glfwMakeContextCurrent(glfw_window);
    }

    void setTitle(const char *title) {
        glfwSetWindowTitle(glfw_window, title);
    }

    [[nodiscard]] bool shouldClose() const {
        return glfwWindowShouldClose(glfw_window);
    }

    void swapBuffers() {
        glfwSwapBuffers(glfw_window);
    }

    void update() {
        refresh_required = false;
        resize_required = false;
    }

    [[nodiscard]] Vec2I getFrameBufferSize() const {
        return frame_buffer_size;
    }

    [[nodiscard]] bool isRefreshRequried() const {
        return true;
        /*return refresh_required;*/
    }

    [[nodiscard]] bool isResizeRequried() const {
        /*return true;*/
        return resize_required;
    };

    void attentionRequest() {
        glfwRequestWindowAttention(glfw_window);
    }

    ~GlfwSystemWindow() {
        glfwDestroyWindow(glfw_window);
    }
};

} // namespace chat::gui::backends
