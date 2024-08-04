#pragma once

#include "../vec.hpp"
#include "err.hpp"
#include "ptr.hpp"
#include <cstddef>
#include <utility>

namespace chat::gui::backends {

class GraphicsException : public StringException {
  public:
    inline GraphicsException(std::string str_) : StringException(std::move(str_)) {};
};

struct RectPos {
    Vec2F bl_pos, tr_pos;
    float z;
};

class DrawableRect {
  public:
    virtual void draw() = 0;

    virtual void setPosition(const RectPos &pos) = 0;

    virtual void setTransform(const glm::mat3 &mat) = 0;

    // Call only for colored rectangle.
    virtual void setColor(const Color &color) = 0;

    // Call only for textured rectangle.
    // DrawableRect always owns texture data.
    [[nodiscard]] virtual not_null<void *> getRGBA8TextureData() = 0;

    // Call only for textured rectangle.
    // Texture data will be invalidated after call!
    virtual void resizeTexture(Vec2I new_res) = 0;

    // Call only for textured rectangle.
    virtual void flushTexture() = 0;

    // Call only for textured rectangle.
    // pitch - number of texels in row
    virtual size_t getPitch() const = 0;
};

class Renderer {
  public:
    // Create a textured rectangle and register it for rendering.
    // Caller becomes the owner of the created texture, renderer saves weak_ptr.
    [[nodiscard]] virtual shared_ptr<DrawableRect>
    createTexturedRect(const RectPos &pos, const Vec2I size) = 0;

    // Create a colored rectangle and register it for rendering.
    // Caller becomes the owner of the created texture, renderer saves weak_ptr.
    [[nodiscard]] virtual shared_ptr<DrawableRect>
    createColoredRect(const RectPos &pos, const Color &color) = 0;

    virtual void resize(Vec2I drawable_area_size) = 0;

    virtual void draw() = 0;
};

} // namespace chat::gui::backends
