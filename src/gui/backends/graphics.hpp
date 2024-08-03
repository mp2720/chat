#pragma once

#include "../vec.hpp"
#include "err.hpp"
#include "ptr.hpp"
#include <vector>
#include <utility>

namespace chat::gui {

class GraphicsException : public StringException {
  public:
    inline GraphicsException(std::string str_) : StringException(std::move(str_)) {};
};

class TexturedRect {
  public:
    virtual void draw() const = 0;

    virtual std::vector<unsigned char> &getTextureData() = 0;

    virtual void setPosition(Vec2f bl_pos, Vec2f tr_pos) = 0;

    virtual void notifyTextureBufChanged() = 0;

    // Texture's content will be invalidated after calling this method!
    virtual void resize(Vec2i size) = 0;
};

class Renderer {
  public:
    // Create a texture and register it for rendering.
    // Caller becomes the owner of the created texture, renderer saves weak_ptr.
    virtual shared_ptr<TexturedRect> createTexture(Vec2f bl_pos, Vec2f tr_pos, Vec2i size) = 0;

    virtual void resize(Vec2i frame_buf_size) = 0;

    virtual void draw() const = 0;
};

} // namespace chat::gui
