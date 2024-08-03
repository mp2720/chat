#pragma once

#include "../gl_include.h"
#include "../vec.hpp"
#include "graphics.hpp"
#include "ptr.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace chat::gui {

// OpenGL object with automatic deletion.
// GL delete function pointers are provided at runtime by GLEW, so we cannot use templates.
// id must be valid OpenGL object's id or 0 (default value).
// Object will not be deleted if id == 0.
class GlObject {
    using ScalarDeleter = void (*)(GLuint);
    using ArrayDeleter = void (*)(GLsizei, const GLuint *);

    std::function<void(GLuint)> deleter;
    GLuint id_ = 0;

  public:
    GlObject(ScalarDeleter deleter_) noexcept : deleter(deleter_) {};

    GlObject(ArrayDeleter deleter_) noexcept
        : deleter([deleter_](GLuint id) noexcept { deleter_(1, &id); }) {};

    GlObject(GlObject &&other) noexcept {
        *this = std::move(other);
    };

    GlObject &operator=(GlObject &&other) noexcept;

    [[nodiscard]] GLuint &id() noexcept {
        return id_;
    }

    ~GlObject() noexcept {
        if (id_ != 0)
            deleter(id_);
    }

    [[nodiscard]] operator GLuint() const noexcept {
        return id_;
    }
};

class GlShaderProgram {
  private:
    GlObject obj{glDeleteProgram};

    constexpr static size_t INFO_LOG_SIZE = 2048;

    class Shader {
      private:
        GlObject obj{glDeleteShader};

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

    void use() const;

    void setBool(not_null<const char *> name, bool value) const;

    void setInt(not_null<const char *> name, int value) const;

    void setFloat(not_null<const char *> name, float value) const;
};

// TODO: overhaul rectangle rendering, add PBOs, color fill
// TODO: render parallelogram, not rectangle

class GlTexturedRect : public TexturedRect {
  private:
    constexpr static const char *const FRAG_SHADER_SRC = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        
        in vec2 TexCoord;
        
        uniform sampler2D tex;
        
        void main()
        {
        	 FragColor = texture(tex, TexCoord);
        }
        )GLSL";

    constexpr static const char *const VERT_SHADER_SRC = R"GLSL(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        
        out vec2 TexCoord;
        
        void main()
        {
        	gl_Position = vec4(aPos, 1.0);
        	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
        }
        )GLSL";

    constexpr static GLuint INDICES[] = {0, 1, 3, 1, 2, 3};

    constexpr static size_t CHANNELS = 4;

    GlObject vao{glDeleteVertexArrays}, vbo{glDeleteBuffers}, ebo{glDeleteBuffers};
    GlObject obj{glDeleteTextures};
    GlShaderProgram shader_program;
    std::vector<unsigned char> texture_buf;
    Vec2i size;
    std::array<GLfloat, 20> vertices;

  public:
    GlTexturedRect(Vec2f bl_pos, Vec2f tr_pos, Vec2i size);

    void draw() const;

    std::vector<unsigned char> &getTextureData() {
        return texture_buf;
    };

    void setPosition(Vec2f bl_pos, Vec2f tr_pos) {}

    void notifyTextureBufChanged();

    void resize(Vec2i size);
};

class GlRenderer : public Renderer {
  private:
    ColorF clear_color;
    mutable std::vector<weak_ptr<TexturedRect>> textured_rects;

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

    shared_ptr<TexturedRect> createTexture(Vec2f bl_pos, Vec2f tr_pos, Vec2i size);

    void resize(Vec2i frame_buf_size);

    void draw() const;
};

} // namespace chat::gui
