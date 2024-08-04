#pragma once

#include "../gl_include.h"
#include "../vec.hpp"
#include "graphics.hpp"
#include "ptr.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace chat::gui::backends {

namespace gl_details {

using GlScalarObjectDeleter = void (*)(GLuint);
using GlArrayObjectDeleter = void (*)(GLsizei, const GLuint *);

// OpenGL object with automatic exception safe deletion.
//
// GL delete function pointers are provided at runtime by GLEW, so we cannot pass these function as
// a template args.
//
// id must be valid OpenGL object's id or 0 (default value), deleter must be OpenGL deleter function
// or nullptr (default value).
//
// If deleter == nullptr, then id must be equal 0!
//
// Object will not be deleted if id == 0.
template <typename D> class GlObject {
    D deleter = nullptr;
    GLuint id_ = 0;

    void delete_() noexcept;

  public:
    static_assert(
        std::is_same_v<D, GlScalarObjectDeleter> || std::is_same_v<D, GlArrayObjectDeleter>,
        "invalid GL object deleter type"
    );

    GlObject() noexcept {};

    GlObject(not_null<D> deleter_) noexcept : deleter(deleter_) {};

    GlObject(GlObject &&other) noexcept {
        *this = std::move(other);
    };

    GlObject &operator=(GlObject &&other) noexcept {
        if (this != &other) {
            this->~GlObject();

            deleter = other.deleter;
            id_ = other.id_;

            other.id_ = 0;
            other.deleter = nullptr;
        }
        return *this;
    };

    [[nodiscard]] GLuint &id() noexcept {
        return id_;
    }

    ~GlObject() noexcept {
        if (id_ != 0) {
            assert(deleter != nullptr && "deleter must not be nullptr when id != 0");
            delete_();
        }
    }

    [[nodiscard]] operator GLuint() const noexcept {
        return id_;
    }
};

template <> inline void GlObject<GlScalarObjectDeleter>::delete_() noexcept {
    deleter(id_);
}

template <> inline void GlObject<GlArrayObjectDeleter>::delete_() noexcept {
    deleter(1, &id_);
}

using GlScalarObject = GlObject<GlScalarObjectDeleter>;
using GlArrayObject = GlObject<GlArrayObjectDeleter>;

class GlShaderProgram {
  private:
    GlScalarObject obj{glDeleteProgram};

    constexpr static size_t INFO_LOG_SIZE = 2048;

    class Shader {
      private:
        GlScalarObject obj{glDeleteShader};

      public:
        enum class Type { FRAGMENT, VERTEX };

        Shader(not_null<const char *> src, Type type);

        [[nodiscard]] GLuint getId() const noexcept {
            return obj;
        }
    };

    [[nodiscard]] GLint getUniformLocation(not_null<const char *> name) const;

  public:
    GlShaderProgram(not_null<const char *> vertex_src, not_null<const char *> frag_src);

    GlShaderProgram(const GlShaderProgram &) = delete;

    GlShaderProgram(GlShaderProgram &&) = default;

    void use();

    void setUniform(not_null<const char *> name, int value);

    void setUniform(not_null<const char *> name, float value);

    void setUniform(not_null<const char *> name, const glm::mat3 &value);

    void setUniform(not_null<const char *> name, const glm::vec4 &value);

    // ...
};

}; // namespace gl_details

class GlDrawableRect : public DrawableRect {
  private:
    constexpr static std::array<GLuint, 6> INDICES = {0, 1, 3, 1, 2, 3};
    constexpr static size_t TEXTURE_CHANNELS = 4;
    constexpr static size_t VERTICES = 4, VERTICES_ARRAY_MAX_SIZE = VERTICES * (3 + 2);

    gl_details::GlArrayObject vao{glDeleteVertexArrays}, vbo{glDeleteBuffers}, ebo{glDeleteBuffers};
    std::array<GLfloat, VERTICES_ARRAY_MAX_SIZE> vertices;
    glm::mat3 transform_mat{1};

    gl_details::GlShaderProgram &shader_program;

    ColorF color;

    gl_details::GlArrayObject texture{glDeleteTextures};

    HeapArray<unsigned char> texture_data;
    Vec2I texture_res;
    size_t texture_size;

    // use only after construction!
    bool isTextured() const noexcept {
        return texture_data != nullptr;
    }

    // use only after construction!
    void checkIsTextured() const noexcept {
        assert(isTextured() && "rectangle is not textured");
    }

    // use only after construction!
    void checkIsColored() const noexcept {
        assert(!isTextured() && "rectangle is not colored");
    }

    size_t genVertices(const RectPos &pos, bool is_textured);

  public:
    GlDrawableRect(
        RectPos pos,
        gl_details::GlShaderProgram &shader_program,
        ColorF color,
        Vec2I texture_res,
        float z
    );

    void draw();

    void setPosition(const RectPos &new_pos);

    void setTransform(const glm::mat3 &mat) noexcept {
        transform_mat = mat;
    }

    void setColor(const Color &color_) noexcept {
        checkIsColored();
        color = color_;
    }

    not_null<void *> getRGBA8TextureData() noexcept {
        checkIsTextured();
        return texture_data;
    };

    void resizeTexture(Vec2I size);

    void flushTexture();

    size_t getPitch() const noexcept {
        checkIsTextured();
        return texture_res.x;
    };
};

class GlRenderer : public Renderer {
  private:
    ColorF clear_color;
    std::vector<weak_ptr<DrawableRect>> rects;

    gl_details::GlShaderProgram *colored_shader_prog, *textured_shader_prog;

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
    GlRenderer(Color clear_color, bool enable_debug_log);

    shared_ptr<DrawableRect> createTexturedRect(const RectPos &pos, const Vec2I size);

    shared_ptr<DrawableRect> createColoredRect(const RectPos &pos, const Color &color);

    void resize(Vec2I frame_buf_size);

    void draw();
};

} // namespace chat::gui::backends
