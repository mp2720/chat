#pragma once

#include "err.hpp"
#include "gui/vec.hpp"
#include "ptr.hpp"
#include <chrono>
#include <optional>

namespace chat::gui {

class RendererException : public StringException {
  public:
    RendererException(std::string str)
        : StringException(str) {}
};

struct RectPos {
    Vec2Px top_left, size;
};

class Drawable {
  public:
    virtual void setPosition(const RectPos &pos) = 0;

    virtual void setTransform(const glm::mat3 &transform) = 0;

    virtual void draw() const = 0;
};

class TexturedRect : public Drawable {
  protected:
    virtual void flush() = 0;

    virtual not_null<unsigned char *> getBuf() = 0;

    // Returns true only if texture is in sync.
    virtual bool waitForSync(std::chrono::nanoseconds timeout) = 0;

  public:
    // Exception safe: dtor execution performs a flush unless uncaught exception is thrown between
    // ctor and dtor calls. Note that flush may throw.
    // Do not delete the texture before the `BufferLock` instance destruction!
    class BufferLock {
        TexturedRect *rect = nullptr;
        int uncaught_exceptions_cnt;

      public:
        BufferLock(TexturedRect *rect_)
            : rect(rect_),
              uncaught_exceptions_cnt(std::uncaught_exceptions()) {}

        BufferLock(BufferLock &&other) {
            *this = std::move(other);
        }

        BufferLock &operator=(BufferLock &&other) {
            if (this != &other) {
                this->~BufferLock();

                this->uncaught_exceptions_cnt = other.uncaught_exceptions_cnt;
                this->rect = other.rect;

                other.rect = nullptr;
            }
            return *this;
        }

        not_null<unsigned char *> get() {
            return rect->getBuf();
        }

        ~BufferLock() {
            if (rect != nullptr && uncaught_exceptions_cnt >= std::uncaught_exceptions())
                rect->flush();
        }
    };

    virtual ~TexturedRect() {}

    // Pitch is a number of texels in row.
    virtual size_t getPitch() const = 0;

    virtual Vec2Px getResolution() const = 0;

    // Texture buf will contain undefined values after this call.
    virtual void resize(Vec2Px new_res) = 0;

    // Get the texture buffer for reading.
    // Doesn't lead to flushing on destruction.
    //
    // If you need to edit texture's content, use `lockBuf()`.
    virtual not_null<const unsigned char *> getConstBuf() const = 0;

    // Get buffer locker that will automatically flush the texture's content on destruction.
    // Note that the fact that timeout is in nanoseconds does not imply that this function has true
    // nanosecond granularity in its timeout; you are only guaranteed that at least that much time
    // will pass.
    [[nodiscard]]
    std::optional<BufferLock> lockBuf(std::chrono::nanoseconds timeout) {
        if (!waitForSync(timeout))
            return {};

        return BufferLock{this};
    }

    [[nodiscard]]
    std::optional<BufferLock> lockBuf() {
        return lockBuf(std::chrono::nanoseconds(0));
    }
};

class ColoredRect : public Drawable {
  public:
    virtual ~ColoredRect() {};

    virtual void setBackgroundBlurRadius(float radius) = 0;

    virtual void setColor(const Color &color) = 0;
};

enum class TextureMode { STATIC, TEXTURE };

class DrawableCreator {
  public:
    virtual ~DrawableCreator() {}

    [[nodiscard]]
    virtual unique_ptr<TexturedRect>
    createBitmapTexturedRect(const RectPos &pos, Vec2Px res, TextureMode mode) = 0;

    // TODO: implement
    /*virtual unique_ptr<TexturedRect> createSvgTexturedRect() = 0;*/

    // TODO: implement
    /*virtual unique_ptr<Text> createText() = 0;*/

    [[nodiscard]]
    virtual unique_ptr<ColoredRect>
    createColoredRect(const RectPos &pos, Color color, float background_blur_radius) = 0;
};

class Renderer : public DrawableCreator {
  public:
    virtual ~Renderer() {}

    virtual void clear() const = 0;

    virtual void resize(Vec2Px size) = 0;
};

struct RendererConfig {
    bool enable_debug_log = false, enable_blur = true;
};

// By declaring `makeXXXRenderer()` methods here we can avoid exposing implementation-dependent
// headers.
// These methods should never return null.

// Never returns null
[[nodiscard]]
unique_ptr<Renderer> makeGlRenderer(const RendererConfig &config);

} // namespace chat::gui
