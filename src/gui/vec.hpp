#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace chat::gui {

using Vec2I = glm::vec<2, int>;
using Vec2F = glm::vec<2, float>;

using Color = glm::vec<4, uint8_t>;
using ColorF = glm::vec<4, float>;

constexpr ColorF colorToF(Color colorb) {
    return ColorF(
        (float)colorb.x / 256,
        (float)colorb.y / 256,
        (float)colorb.z / 256,
        (float)colorb.w / 256
    );
}

} // namespace chat::gui
