#pragma once

#include "../vec.hpp"
#include "err.hpp"
#include <string>

namespace chat::gui {

class IoException : public StringException {
  public:
    inline IoException(std::string str_) : StringException(std::move(str_)) {};
};

class SystemWindow {
  public:
    // TODO: window icon

    virtual void setTitle(const char *title) = 0;

    // Switch context to this window.
    virtual void switchContext() = 0;

    [[nodiscard]] virtual bool shouldClose() const = 0;

    virtual void swapBuffers() = 0;

    // Process all queued events.
    virtual void update() = 0;

    [[nodiscard]] virtual Vec2i getFrameBufferSize() const = 0;

    // True iff window is resized or its content is damaged.
    // See also isResizeRequried().
    [[nodiscard]] virtual bool isRefreshRequried() const = 0;

    // True iff window is resized.
    [[nodiscard]] virtual bool isResizeRequried() const = 0;

    // Request an attention.
    virtual void attentionRequest() = 0;
};

} // namespace chat::gui
