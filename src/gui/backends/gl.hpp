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

class GlRenderer final : public Renderer {
    Vec2Px size;
    GlShaderProgramsManager shader_progs_manager;
    bool enable_blur;

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
    GlRenderer(bool enable_debug_log, bool enable_blur);

    void clear() const final;

    void resize(Vec2Px size) final;

    unique_ptr<TexturedRect>
    createBitmapTexturedRect(const RectPos &pos, Vec2Px res, TextureMode mode) final;

    unique_ptr<ColoredRect>
    createColoredRect(const RectPos &pos, Color color, float background_blur_radius) final;

    Vec2Px getRes() const noexcept {
        return size;
    }

    const GlShaderProgramsManager &getShaderProgramsManager() const noexcept {
        return shader_progs_manager;
    };
};

class GlViewport {
    GlRenderer &renderer;

  public:
    GlViewport(GlRenderer &renderer_)
        : renderer(renderer_) {};

    glm::vec2 pxToGlCoords(Vec2Px pos) const noexcept {
        return glm::vec2{pos.x, pos.y} / glm::vec2{renderer.getRes().x, renderer.getRes().y};
    }
};

// Represents single OpenGL polygon without shaders.
class GlRectPolygon {
    constexpr static std::array<GLuint, 6> INDICES = {0, 1, 3, 1, 2, 3};
    constexpr static size_t VERTICES = 4;

    constexpr static size_t POS_COORDS_NUM = 3;
    constexpr static size_t TEX_COORDS_NUM = 2;

    constexpr static size_t TEX_VERTICES_ARRAY_SIZE = VERTICES * (POS_COORDS_NUM + TEX_COORDS_NUM);
    constexpr static size_t NOTEX_VERTICES_ARRAY_SIZE = VERTICES * POS_COORDS_NUM;

    constexpr static size_t VERTICES_ARRAY_MAX_SIZE = TEX_VERTICES_ARRAY_SIZE;

    constexpr static auto Z_DEPTH = 256;

    bool has_texture_coords;
    GlObject<&glDeleteVertexArrays> vao;
    GlObject<&glDeleteBuffers> vbo, ebo;
    glm::mat3 transform_mat{1};
    std::array<GLfloat, VERTICES_ARRAY_MAX_SIZE> vertices;

    void genVertices(const GlViewport &viewport, const RectPos &pos) noexcept;

  public:
    GlRectPolygon(const GlViewport &viewport, const RectPos &pos, bool add_texture_coords);

    void draw() const;

    void setPosition(const GlViewport &viewport, const RectPos &new_pos);

    void setTransform(const glm::mat3 &mat) noexcept {
        transform_mat = mat;
    }
};

// TODO: add streaming texture support
class GlTexturedRect final : public TexturedRect {
    constexpr static size_t TEXTURE_CHANNELS = 4;

    GlRenderer &renderer;
    GlObject<&glDeleteTextures> texure;
    GlRectPolygon polygon;
    unique_ptr<unsigned char[]> buf;
    size_t buf_capacity = 0;
    Vec2Px res;
    TextureMode mode;

    void flush() final;

    not_null<unsigned char *> getBuf() noexcept final {
        return buf.get();
    };

    bool waitForSync(std::chrono::nanoseconds timeout) final;

  public:
    GlTexturedRect(GlRenderer &renderer, const RectPos &pos, Vec2Px res, TextureMode mode);

    void setPosition(const RectPos &pos) final {
        polygon.setPosition(renderer, pos);
    };

    void setTransform(const glm::mat3 &transform) final {
        polygon.setTransform(transform);
    }

    void draw() const final;

    size_t getPitch() const noexcept final {
        return res.x;
    };

    Vec2Px getResolution() const noexcept final {
        return res;
    };

    void resize(Vec2Px new_res) final;

    not_null<const unsigned char *> getConstBuf() const noexcept final {
        return buf.get();
    };
};

class GlColoredRect final : public ColoredRect {
    GlRenderer &renderer;
    GlRectPolygon polygon;
    ColorF color;
    float bg_blur_radius;

  public:
    GlColoredRect(GlRenderer &renderer_, const RectPos &pos, Color color_, float bg_blur_radius_)
        : renderer{renderer_},
          polygon{GlViewport{renderer}, pos, false},
          color{color_},
          bg_blur_radius{bg_blur_radius_} {};

    void setPosition(const RectPos &pos) final {
        polygon.setPosition(renderer, pos);
    }

    void setTransform(const glm::mat3 &mat) final {
        polygon.setTransform(mat);
    }

    void draw() const final;

    void setColor(const Color &color) noexcept final {
        this->color = colorToF(color);
    }

    void setBackgroundBlurRadius(float value) noexcept final {
        bg_blur_radius = value;
    }
};

} // namespace chat::gui::backends
