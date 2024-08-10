#pragma once

#include "err.hpp"
#include "gui/renderer.hpp"
#include "ptr.hpp"
#include <string>

namespace chat::gui {

class WindowException : public StringException {
  public:
    inline WindowException(std::string str_)
        : StringException(std::move(str_)) {};
};

class Window {
  public:
    virtual void setTitle(const char *title) = 0;

    virtual void requestAttention() = 0;

    virtual ~Window() {}
};

class WindowSystem {
  public:
    [[nodiscard]]
    virtual weak_ptr<Window> createWindow(const RendererConfig &config) = 0;

    // Does nothing if `window` is already expired.
    virtual void closeWindow(weak_ptr<Window> window) = 0;

    // Runs the "handle events then draw" loop. Blocks the main thread until all windows are closed.
    virtual void runLoop() = 0;

    virtual ~WindowSystem() {}
};

[[nodiscard]]
unique_ptr<WindowSystem> makeGlfwWindowSystem();

} // namespace chat::gui
