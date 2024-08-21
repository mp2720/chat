#pragma once

#include "gui/vec.hpp"
#include "renderer.hpp"
#include "win.hpp"

namespace chat::gui {

struct Context {
    not_null<DrawableCreator *> drawable_creator;
    not_null<Window *> window;
};

enum class UnitType { PX, PT };

enum class UnitAxis { VERTICAL, HORIZONTAL };

template <UnitAxis axis> struct Unit {
    float value;
    UnitType type;

    Unit(float value_, UnitType type_)
        : value(value_),
          type(type_) {};

    float toPx(const Context &ctx) const noexcept {
        switch (type) {
        case UnitType::PX:
            return value;
        case UnitType::PT:
            return value * getScaleForAxis(ctx);
        default:
            assert(false && "invalid unit");
        }
    }

  private:
    float getScaleForAxis(const Context &ctx) const;
};

template <> inline float Unit<UnitAxis::HORIZONTAL>::getScaleForAxis(const Context &ctx) const {
    return ctx.window->getScale().x;
}

template <> inline float Unit<UnitAxis::VERTICAL>::getScaleForAxis(const Context &ctx) const {
    return ctx.window->getScale().y;
}

struct Vec2Unit {
    Unit<UnitAxis::HORIZONTAL> x;
    Unit<UnitAxis::VERTICAL> y;

    Vec2Px toPx(const Context &ctx) const {
        return {x.toPx(ctx), y.toPx(ctx)};
    }
};

class Widget {
    Vec2Unit min_size, max_size;

  protected:
    virtual Vec2Px calcContentSize() = 0;

  public:
    Widget(Vec2Unit min_size_, Vec2Unit max_size_)
        : min_size(min_size_),
          max_size(max_size_) {}

    virtual ~Widget() {}

    virtual void draw(const Context &context) = 0;

    virtual void resize(Vec2Px parent_size) = 0;

    Vec2Px calcSize();
};

} // namespace chat::gui
