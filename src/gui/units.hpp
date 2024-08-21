#pragma once

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>

namespace chat::gui {

using Px = int;
using Pt = float;

using Vec2Pt = glm::vec<2, Pt>;
using Vec2Px = glm::vec<2, Px>;
using Vec2F = glm::vec2;

struct Viewport {
    Vec2Px size;
    Vec2F scale;

    Viewport(Vec2Px size_, Vec2F scale_)
        : size(size_),
          scale(scale_) {}
};

enum class UnitType { PX, PT };

struct Unit {
    UnitType type = UnitType::PX;

    union {
        Px px = 0;
        Pt pt;
    } value;

    Unit() = default;

    static Unit fromPx(Px px) noexcept {
        return Unit{UnitType::PX, {.px = px}};
    }

    static Unit fromPt(Pt pt) noexcept {
        return Unit{UnitType::PX, {.pt = pt}};
    }

    Px toPx(float scale) const noexcept {
        switch (type) {
        case UnitType::PX:
            return value.px;
        case UnitType::PT:
            return static_cast<int>(std::round(value.pt * scale));
        default:
            assert(false && "invalid unit");
        }
    }

  private:
    float getScaleForAxis(const Viewport &viewport) const;
};

struct Vec2Unit {
    UnitH x;
    UnitV y;

    Vec2Unit(UnitH x_, UnitV y_)
        : x(x_),
          y(y_) {}
};

using Color = glm::vec<4, uint8_t>;
using ColorF = glm::vec<4, float>;

inline UnitH

    constexpr ColorF
    colorToF(Color colorb) {
    return ColorF(
        (float)colorb.x / 256,
        (float)colorb.y / 256,
        (float)colorb.z / 256,
        (float)colorb.w / 256
    );
}

} // namespace chat::gui
