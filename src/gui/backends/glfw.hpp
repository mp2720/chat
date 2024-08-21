#pragma once

#include "../win.hpp"
#include "gui/renderer.hpp"
#include "gui/vec.hpp"
#include "ptr.hpp"
#include <GLFW/glfw3.h>
#include <vector>

using GlfwCWindowHandle = GLFWwindow *;

namespace chat::gui::backends {

class GlfwWindow final : public Window {
  private:
    GlfwCWindowHandle win_handle;
    unique_ptr<Renderer> renderer_ctx;
    bool refresh_required = true;
    bool resize_required = false;
    bool need_redraw = false;
    Vec2Px size;
    glm::vec2 win_scale;
    float ui_scale;

    void updateSizeAndScale() {
        glfwGetFramebufferSize(win_handle, &size.x, &size.y);
        glfwGetWindowContentScale(win_handle, &win_scale.x, &win_scale.y);
    }

  public:
    GlfwWindow(const RendererConfig &config, float ui_scale);

    void setTitle(const char *title) final {
        glfwSetWindowTitle(win_handle, title);
    }

    void requestAttention() final {
        glfwRequestWindowAttention(win_handle);
    };

    Vec2Px getSize() const final {
        return size;
    };

    float getUiScale() const final {
        return ui_scale;
    }

    glm::vec2 getWindowScale() const final {
        return win_scale;
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

    static void onWindowRefresh(GlfwCWindowHandle win_handle) noexcept;

    static void onWindowResize(GlfwCWindowHandle win_handle, int width, int height) noexcept;

    static void
    onWindowScaleChange(GlfwCWindowHandle win_handle, float xscale, float yscale) noexcept;

  public:
    GlfwWindowSystem();

    [[nodiscard]]
    weak_ptr<Window> createWindow(const RendererConfig &renderer_config, float ui_scale) final;

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
