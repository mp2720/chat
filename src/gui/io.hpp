#pragma once

#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>

namespace chat::gui {

class Io {
    GLFWwindow *window;
    constexpr static glm::vec4 CLEAR_COLOR{1, 0, 0, 1};

  public:
    void init();
    bool shouldContinue();
    void pollEvents();
    void drawFrameStart();
    void drawFrameEnd();
    void terminate();
};

} // namespace chat::gui
