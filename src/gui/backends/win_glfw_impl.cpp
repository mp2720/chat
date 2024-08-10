#include "win_glfw_impl.hpp"

#include "../renderer.hpp"
#include "gui/win.hpp"
#include "log.hpp"
#include <GLFW/glfw3.h>
#include <boost/format.hpp>
#include <memory>

using namespace chat::gui::backends;

using boost::format, boost::str;
using chat::weak_ptr, chat::unique_ptr, chat::not_null;
using chat::gui::Window, chat::gui::WindowSystem;

GlfwWindow::GlfwWindow(const RendererConfig &config) {
    // This specific title is needed to configure WMs (like i3) to move window to another
    // workspace. i3 need this title to be set during the window creation. The actual title can be
    // set later using setTitle().
    win_handle = glfwCreateWindow(1280, 720, "mp2720/chat", nullptr, nullptr);
    if (win_handle == nullptr)
        GlfwWindowSystem::handleError();

    glfwSetWindowUserPointer(win_handle, this);
    GlfwWindowSystem::setCallbacksForWindow(win_handle);

    switchContext();

    // vsync
    glfwSwapInterval(1);

    // call only after creating GLFW window and switching context.
    renderer_ctx = makeGlRendererContext(config);
}

void GlfwWindow::update() {
    switchContext();

    if (resize_required) {
        Vec2I fb_size;
        glfwGetFramebufferSize(win_handle, &fb_size.x, &fb_size.y);
        renderer_ctx->resize(fb_size);
        resize_required = false;
    }

    renderer_ctx->drawStart();

    glfwSwapBuffers(win_handle);
}

void GlfwWindowSystem::onError(int error, const char *description) noexcept {
    CHAT_LOG_FILE_LINE(
        Logger::ERROR,
        nullptr,
        0,
        format("GLFW error %1% %2%") % error % description
    );
}

void GlfwWindowSystem::onWindowRefresh(GlfwCWindowHandle window) noexcept {
    auto &this_ = getWindow(window);
    this_.requireRefresh();
}

void GlfwWindowSystem::onWindowResize(
    GlfwCWindowHandle window,
    [[maybe_unused]] int width,
    [[maybe_unused]] int height
) noexcept {
    auto &this_ = getWindow(window);
    this_.requireResize();
}

void GlfwWindowSystem::handleError() {
    const char *description;
    int code = glfwGetError(&description);

    throw WindowException(str(format("GLFW error %1% %2%") % code % description));
}

GlfwWindowSystem::GlfwWindowSystem() {
    if (glfwInit() != GLFW_TRUE)
        handleError();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwSetErrorCallback(onError);
}

[[nodiscard]]
weak_ptr<Window> GlfwWindowSystem::createWindow(const RendererConfig &config) {
    auto ptr = std::make_shared<GlfwWindow>(config);
    windows.emplace_back(ptr);
    return ptr;
};

void GlfwWindowSystem::closeWindow(weak_ptr<Window> window) {
    auto lock = window.lock();
    if (!lock)
        return;

    auto it = std::find(windows.begin(), windows.end(), lock);
    assert(it != windows.end() && "window is not found in windows list");

    windows.erase(it);
}

void GlfwWindowSystem::runLoop() {
    while (!windows.empty()) {
        bool need_redraw = false;

        for (auto it = windows.begin(); it != windows.end();) {
            if ((*it)->shouldClose()) {
                it = windows.erase(it);
                continue;
            }

            (*it)->update();
            need_redraw = need_redraw || (*it)->needRedraw();

            ++it;
        }

        if (need_redraw) {
            glfwPollEvents();
            continue;
        }

        glfwWaitEvents();
    }
}

void GlfwWindowSystem::setCallbacksForWindow(GlfwCWindowHandle win_handle) {
    glfwSetFramebufferSizeCallback(win_handle, onWindowResize);
    glfwSetWindowRefreshCallback(win_handle, onWindowRefresh);
}

unique_ptr<WindowSystem> chat::gui::makeGlfwWindowSystem() {
    return std::make_unique<GlfwWindowSystem>();
}
