#pragma once

#include "backends/graphics.hpp"
#include "backends/sys.hpp"
#include "ptr.hpp"
#include <chrono>

namespace chat::gui {

class Window {
  private:
    unique_ptr<backends::SystemWindow> system_window;
    unique_ptr<backends::Renderer> renderer;
    shared_ptr<backends::DrawableRect> rect1, rect2;

    bool need_redraw = true;
    bool can_continue = true;
    // for fps count
    std::chrono::milliseconds prev_update_time;

    void resize();

  public:
    Window(
        unique_ptr<backends::SystemWindow> system_window_,
        unique_ptr<backends::Renderer> renderer_
    );

    void update();

    [[nodiscard]] bool needRedraw() const noexcept {
        return need_redraw;
    }

    [[nodiscard]] bool canContinue() const noexcept {
        return can_continue;
    }
};

} // namespace chat::gui
