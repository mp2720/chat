#pragma once

#include "err.hpp"
#include "ptr.hpp"
#include "vec.hpp"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <exception>
#include <optional>
#include <utility>

namespace chat::gui {

class GraphicsException : public StringException {
  public:
    inline GraphicsException(std::string str_)
        : StringException(std::move(str_)) {};
};

struct RectPos {
    Vec2F bl, tr;
};

class Texture {
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
        Texture *texture = nullptr;
        int uncaught_exceptions_cnt;

      public:
        BufferLock(Texture *texture_)
            : texture(texture_),
              uncaught_exceptions_cnt(std::uncaught_exceptions()) {}

        BufferLock(BufferLock &&other) {
            *this = std::move(other);
        }

        BufferLock &operator=(BufferLock &&other) {
            if (this != &other) {
                this->~BufferLock();

                this->uncaught_exceptions_cnt = other.uncaught_exceptions_cnt;
                this->texture = other.texture;

                other.texture = nullptr;
            }
            return *this;
        }

        not_null<unsigned char *> get() {
            return texture->getBuf();
        }

        ~BufferLock() {
            if (texture != nullptr && uncaught_exceptions_cnt >= std::uncaught_exceptions())
                texture->flush();
        }
    };

    // Pitch is a number of texels in row.
    virtual size_t getPitch() const = 0;

    virtual Vec2I getResolution() const = 0;

    // Texture buf will contain undefined values after this call.
    virtual void resize(Vec2I new_res) = 0;

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

    virtual ~Texture() {}
};

class DrawableRect {
  public:
    virtual void setPosition(const RectPos &pos) = 0;

    virtual void setTransform(const glm::mat3 &mat) = 0;

    // Takes effect only if NO_TEXTURE is a texture mode.
    virtual void setColor(const Color &color) = 0;

    virtual void setBackgroundBlurRadius(float value) = 0;

    // returns null only if NO_TEXTURE is a texture mode
    virtual Texture *getTexture() = 0;

    // returns null only if NO_TEXTURE is a texture mode
    virtual const Texture *getConstTexture() const = 0;

    not_null<Texture *> requireTexture() {
        return getTexture();
    }

    not_null<const Texture *> requireConstTexture() const {
        return getConstTexture();
    }

    virtual void draw() const = 0;

    virtual ~DrawableRect() {}
};

enum class TextureMode {
    NO_TEXTURE,
    STATIC_TEXTURE,
    /*STREAMING_TEXTURE,*/
};

struct DrawableRectConfig {
    RectPos pos{};

    TextureMode texture_mode = TextureMode::NO_TEXTURE;
    Vec2I texture_res{};

    Color color{};

    float bg_blur_radius = 0;
};

// Do not delete instance of this class if any `DrawableRect` or `DrawableRectCreator` pointing to
// this instance is alive.
class RendererContext {
  public:
    // Create drawable rectangle that is ready for rendering by calling `draw()` method.
    // Never returns null.
    [[nodiscard]]
    virtual unique_ptr<DrawableRect> createRect(const DrawableRectConfig &conf) = 0;

    virtual void resize(Vec2I drawable_area_size) = 0;

    virtual void drawStart() const = 0;

    virtual ~RendererContext() {}
};

// Wrapper around the `RendererContext` that allows only `DrawableRect` creation and hides other
// virtual methods.
// Never returns nullptr.
class DrawableRectCreator {
  private:
    not_null<RendererContext *> ctx;

  public:
    DrawableRectCreator(not_null<RendererContext *> ctx_)
        : ctx(ctx_) {};

    unique_ptr<DrawableRect> operator()(const DrawableRectConfig &conf) {
        return ctx->createRect(conf);
    }
};

struct RendererConfig {
    Color clear_color{0, 0, 0, 1};
    bool enable_debug_log = false, enable_blur = true;
};

// By declaring `createXXXRendererContext()` methods here we can avoid exposing
// implementation-dependent headers.
// These methods should never return null.

// defined in `renderer_gl_impl.cpp`
// Never returns null
[[nodiscard]]
unique_ptr<RendererContext> makeGlRendererContext(const RendererConfig &config);

} // namespace chat::gui
