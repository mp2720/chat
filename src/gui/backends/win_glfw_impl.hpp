#pragma once

#include "../win.hpp"
#include "gui/renderer.hpp"
#include "ptr.hpp"
#include <GLFW/glfw3.h>
#include <vector>

using GlfwCWindowHandle = GLFWwindow *;

namespace chat::gui::backends {

class GlfwWindow final : public Window {
  private:
    GlfwCWindowHandle win_handle;
    unique_ptr<RendererContext> renderer_ctx;
    bool refresh_required = true;
    bool resize_required = false;
    bool need_redraw = false;

  public:
    GlfwWindow(const RendererConfig &config);

    void setTitle(const char *title) final {
        glfwSetWindowTitle(win_handle, title);
    }

    void requestAttention() final {
        glfwRequestWindowAttention(win_handle);
    };

    ~GlfwWindow() final {
        glfwDestroyWindow(win_handle);
    }

    void update();

    bool shouldClose() const {
        return glfwWindowShouldClose(win_handle);
    }

    void requireRefresh() noexcept {
        refresh_required = true;
    }

    void requireResize() noexcept {
        resize_required = true;
    }

    void switchContext() {
        glfwMakeContextCurrent(win_handle);
    }

    bool needRedraw() const noexcept {
        return need_redraw;
    }
};

class GlfwWindowSystem final : public WindowSystem {
    std::vector<shared_ptr<GlfwWindow>> windows;

    // Callbacks for GLFW. Should never throw (GLFW is a C library), because it causes UB.

    static GlfwWindow &getWindow(GlfwCWindowHandle window) noexcept {
        return *reinterpret_cast<GlfwWindow *>(window);
    }

    static void onError(int error, const char *description) noexcept;

    static void onWindowRefresh(GlfwCWindowHandle window) noexcept;

    static void onWindowResize(GlfwCWindowHandle window, int width, int height) noexcept;

  public:
    GlfwWindowSystem();

    [[nodiscard]]
    weak_ptr<Window> createWindow(const RendererConfig &config) final;

    void closeWindow(weak_ptr<Window> window) final;

    void runLoop() final;

    ~GlfwWindowSystem() final {
        glfwTerminate();
    }

    // Handle GLFW error code from the last called function.
    static void handleError();

    static void setCallbacksForWindow(GlfwCWindowHandle win_handle);
};

} // namespace chat::gui::backends
