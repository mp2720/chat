#pragma once

#include "../renderer.hpp"
#include "../vec.hpp"
#include "GL/glew.h"
#include "ptr.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace chat::gui::backends {

class GlRendererContext;

namespace gl_details {

using GlSingleDeleteFun = void(GLuint);
using GlArrayDeleteFun = void(GLsizei, const GLuint *);

template <typename F> struct GlDeleteFunCaller {
    void operator()(F fun, GLuint id);
};

// For old GL functions (like `glClear()`) that have known address at compile-time.

template <>
inline void GlDeleteFunCaller<GlSingleDeleteFun *>::operator()(GlSingleDeleteFun *fun, GLuint id) {
    fun(id);
}

template <>
inline void GlDeleteFunCaller<GlArrayDeleteFun *>::operator()(GlArrayDeleteFun *fun, GLuint id) {
    fun(1, &id);
}

// For more modern GL functions (like `glDeleteShader()`) that have unknown address at compile-time.

template <>
inline void
GlDeleteFunCaller<GlSingleDeleteFun **>::operator()(GlSingleDeleteFun **fun, GLuint id) {
    (*fun)(id);
}

template <>
inline void GlDeleteFunCaller<GlArrayDeleteFun **>::operator()(GlArrayDeleteFun **fun, GLuint id) {
    (*fun)(1, &id);
}

template <auto deleteFunPtr> class GlObject {
  private:
    GLuint id = 0;

    using DeleteFunType = std::remove_pointer_t<std::remove_pointer_t<decltype(deleteFunPtr)>>;

    static_assert(
        std::is_same_v<DeleteFunType, GlSingleDeleteFun> ||
            std::is_same_v<DeleteFunType, GlArrayDeleteFun>,
        "invalid GL object deleter type"
    );

  public:
    GlObject() noexcept {};

    GlObject(GlObject &&other) noexcept {
        *this = std::move(other);
    };

    GlObject &operator=(GlObject &&other) noexcept {
        if (this != &other) {
            this->~GlObject();

            id = other.id;

            other.id = 0;
        }
        return *this;
    };

    ~GlObject() noexcept {
        if (id != 0) {
            GlDeleteFunCaller<decltype(deleteFunPtr)> caller;
            caller(deleteFunPtr, id);
        }
    }

    GLuint getId() const noexcept {
        return id;
    }

    GLuint &getIdRef() noexcept {
        return id;
    }

    operator GLuint() const noexcept {
        return id;
    }
};

class GlShaderProgram {
  private:
    GlObject<&glDeleteProgram> obj;

    constexpr static size_t INFO_LOG_SIZE = 2048;

    class Shader {
      private:
        GlObject<&glDeleteShader> obj;

      public:
        enum class Type { FRAGMENT, VERTEX };

        Shader(not_null<const char *> src, Type type);

        GLuint getId() const noexcept {
            return obj.getId();
        }
    };

    GLint getUniformLocation(not_null<const char *> name) const;

  public:
    GlShaderProgram() {};

    GlShaderProgram(not_null<const char *> vertex_src, not_null<const char *> frag_src);

    GlShaderProgram(GlShaderProgram &&other) noexcept {
        *this = std::move(other);
    };

    GlShaderProgram &operator=(GlShaderProgram &&other) noexcept {
        this->obj = std::move(other.obj);
        return *this;
    };

    void use() const;

    void setUniform(not_null<const char *> name, int value) const;

    void setUniform(not_null<const char *> name, float value) const;

    void setUniform(not_null<const char *> name, const glm::mat3 &value) const;

    void setUniform(not_null<const char *> name, const glm::vec4 &value) const;

    // ...
};

class GlShaderProgramsManager {
  private:
    GlShaderProgram colored, textured_argb8888;

  public:
    void init();

    const GlShaderProgram &getColored() const noexcept {
        return colored;
    };

    const GlShaderProgram &getTextured() const noexcept {
        return textured_argb8888;
    }
};

// Represents single OpenGL polygon without shaders.
class GlRectPolygon {
  private:
    constexpr static std::array<GLuint, 6> INDICES = {0, 1, 3, 1, 2, 3};
    constexpr static size_t VERTICES = 4;

    constexpr static size_t POS_COORDS_NUM = 3;
    constexpr static size_t TEX_COORDS_NUM = 2;

    constexpr static size_t TEX_VERTICES_ARRAY_SIZE = VERTICES * (POS_COORDS_NUM + TEX_COORDS_NUM);
    constexpr static size_t NOTEX_VERTICES_ARRAY_SIZE = VERTICES * POS_COORDS_NUM;

    constexpr static size_t VERTICES_ARRAY_MAX_SIZE = TEX_VERTICES_ARRAY_SIZE;

    constexpr static auto Z_DEPTH = 256;

    GlObject<&glDeleteVertexArrays> vao;
    GlObject<&glDeleteBuffers> vbo, ebo;

    glm::mat3 transform_mat{1};
    std::array<GLfloat, VERTICES_ARRAY_MAX_SIZE> vertices;

    void genVertices(const RectPos &pos, bool add_texture_coords) noexcept;

  public:
    GlRectPolygon(const RectPos &pos, bool add_texture_coords);

    void draw() const;

    // Note: this method does not update the Z order.
    void setPosition(const RectPos &new_pos, bool add_texture_coords);

    // Note: this method does not update the Z order.
    void setTransform(const glm::mat3 &mat) noexcept {
        transform_mat = mat;
    }
};

// TODO: add streaming texture support
class GlTexture final : public Texture {
    constexpr static size_t TEXTURE_CHANNELS = 4;

    GlObject<&glDeleteTextures> obj;
    unique_ptr<unsigned char[]> buf;
    size_t buf_capacity = 0;
    Vec2I res;
    TextureMode mode;

    // === Texture implementation

    void flush() final;

    not_null<unsigned char *> getBuf() noexcept final {
        return buf.get();
    };

    bool waitForSync(std::chrono::nanoseconds timeout) final;

  public:
    size_t getPitch() const noexcept final {
        return res.x;
    };

    Vec2I getResolution() const noexcept final {
        return res;
    };

    void resize(Vec2I new_res) final;

    not_null<const unsigned char *> getConstBuf() const noexcept final {
        return buf.get();
    };

    // === other

    GlTexture(Vec2I res, TextureMode mode);

    // Check before calling any other method outside of the class.
    // Also check before exposing texture's ptr.
    bool isEnabled() const noexcept {
        return mode != TextureMode::NO_TEXTURE;
    }

    void prepareShader(const GlShaderProgramsManager &shp_man) const;
};

class GlDrawableRect final : public DrawableRect {
  private:
    GlRendererContext &ctx;
    GlTexture texture;
    GlRectPolygon polygon;
    ColorF color;
    float bg_blur_radius;
    glm::mat4 transform_mat{1};

  public:
    GlDrawableRect(GlRendererContext &renderer, const DrawableRectConfig &config);

    // === DrawableRect implementation

    void setPosition(const RectPos &pos) final {
        polygon.setPosition(pos, texture.isEnabled());
    }

    void setTransform(const glm::mat3 &mat) final {
        transform_mat = mat;
    }

    void setColor(const Color &color) noexcept final {
        this->color = color;
    }

    void setBackgroundBlurRadius(float value) noexcept final {
        // TODO: implement blur
        bg_blur_radius = value;
    }

    Texture *getTexture() noexcept final {
        return texture.isEnabled() ? &texture : nullptr;
    }

    const Texture *getConstTexture() const noexcept final {
        return texture.isEnabled() ? &texture : nullptr;
    };

    void draw() const final;
};

}; // namespace gl_details

class GlRendererContext final : public RendererContext {
  private:
    ColorF clear_color;
    bool enable_blur;
    gl_details::GlShaderProgramsManager shader_programs_manager;

    static void handleGlewError(GLenum err);

    static void onGlErrorCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar *message,
        const void *user_param
    );

  public:
    GlRendererContext(Color clear_color, bool enable_debug_log, bool enable_blur);

    const gl_details::GlShaderProgramsManager &getShaderProgramsManager() const noexcept {
        return shader_programs_manager;
    }

    // === Renderer implementation

    [[nodiscard]]
    unique_ptr<DrawableRect> createRect(const DrawableRectConfig &conf) final;

    void resize(Vec2I drawable_area_size) final;

    void drawStart() const final;
};

} // namespace chat::gui::backends
