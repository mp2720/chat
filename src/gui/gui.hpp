#pragma once

#include "ptr.hpp"
#include "renderer.hpp"
#include "sys.hpp"
#include <chrono>

namespace chat::gui {

class Gui {
  private:
    unique_ptr<Window> system_ctx;
    unique_ptr<RendererContext> renderer_ctx;
    shared_ptr<DrawableRect> rect1, rect2;

    bool need_redraw = true;
    bool can_continue = true;
    // for fps count
    std::chrono::milliseconds prev_update_time;

    void resize();

  public:
    Gui(unique_ptr<Window> system, unique_ptr<RendererContext> renderer_ctx);

    void update();

    [[nodiscard]]
    bool needRedraw() const noexcept {
        return need_redraw;
    }

    [[nodiscard]]
    bool canContinue() const noexcept {
        return can_continue;
    }
};

} // namespace chat::gui
