#include "graphics_gl_impl.hpp"

#include "../gl_include.h"
#include "gui/vec.hpp"
#include "log.hpp"
#include "ptr.hpp"
#include <array>
#include <boost/format.hpp>
#include <cstddef>
#include <cstdint>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

using namespace chat;
using namespace gui;
using namespace backends;
using namespace gl_details;

using boost::str, boost::format;
using chat::Logger;

// TODO: просмотреть вызовы функций, обёрнутые в CHAT_GL_CHECK. Возможно не все из них нужно
// TODO: проверять.
#ifdef CHAT_GL_IGNORE_ERRORS
#  define CHAT_GL_CHECK(stmt) stmt
#else
static void checkGlError(const char *stmt) {
    GLenum err = glGetError();
    if (err == GL_NO_ERROR)
        return;

    // Зачем возвращать const unsigned char * ???
    const char *str = reinterpret_cast<const char *>(glewGetErrorString(err));
    throw GraphicsException(::str(format("GL error %1% in %2%") % str % stmt));
}

#  define CHAT_GL_CHECK(stmt)  \
      do {                     \
          stmt;                \
          checkGlError(#stmt); \
      } while (0)
#endif // CHAT_GL_IGNORE_ERRORS

GlShaderProgram::Shader::Shader(not_null<const char *> src, Type type) {
    const char *type_str;
    GLenum gl_type;

    switch (type) {
    case Type::VERTEX:
        type_str = "vertex";
        gl_type = GL_VERTEX_SHADER;
        break;

    case Type::FRAGMENT:
        type_str = "fragment";
        gl_type = GL_FRAGMENT_SHADER;
        break;
    }

    CHAT_GL_CHECK(obj.getIdRef() = glCreateShader(gl_type));
    if (obj.getIdRef() == 0)
        throw GraphicsException("failed to create shader");

    const char *src_ = src;

    CHAT_GL_CHECK(glShaderSource(obj, 1, &src_, nullptr));
    CHAT_GL_CHECK(glCompileShader(obj));

    int ok;
    CHAT_GL_CHECK(glGetShaderiv(obj, GL_COMPILE_STATUS, &ok));
    if (!ok) {
        char info_log[INFO_LOG_SIZE];
        CHAT_GL_CHECK(glGetShaderInfoLog(obj, sizeof info_log, nullptr, info_log));

        throw GraphicsException(
            str(format("failed to compile %1% shader: %2%") % type_str % info_log)
        );
    }
}

GLint GlShaderProgram::getUniformLocation(not_null<const char *> name) const {
    GLint loc;
    CHAT_GL_CHECK(loc = glGetUniformLocation(obj, name));
    /*if (loc < 0)*/
    /*    throw GraphicsException(str(format("uniform %1% not found") % name));*/

    return loc;
}

GlShaderProgram::GlShaderProgram(
    not_null<const char *> vertex_src,
    not_null<const char *> frag_src
) {

    Shader vertex(vertex_src, Shader::Type::VERTEX), fragment(frag_src, Shader::Type::FRAGMENT);
    CHAT_GL_CHECK(obj.getIdRef() = glCreateProgram());
    if (obj.getIdRef() == 0)
        throw new GraphicsException("failed to create shader program");

    CHAT_GL_CHECK(glAttachShader(obj, vertex.getId()));
    CHAT_GL_CHECK(glAttachShader(obj, fragment.getId()));
    CHAT_GL_CHECK(glLinkProgram(obj));

    int ok;
    CHAT_GL_CHECK(glGetProgramiv(obj, GL_LINK_STATUS, &ok));
    if (!ok) {
        char info_log[INFO_LOG_SIZE];
        CHAT_GL_CHECK(glGetProgramInfoLog(obj, sizeof info_log, nullptr, info_log));

        throw GraphicsException(str(format("failed to link shader program: %2%") % info_log));
    }
}

void GlShaderProgram::use() const {
    CHAT_GL_CHECK(glUseProgram(obj));
}

void GlShaderProgram::setUniform(not_null<const char *> name, int value) const {
    CHAT_GL_CHECK(glUniform1i(getUniformLocation(name), (int)value));
}

void GlShaderProgram::setUniform(not_null<const char *> name, float value) const {
    CHAT_GL_CHECK(glUniform1f(getUniformLocation(name), value));
}

void GlShaderProgram::setUniform(not_null<const char *> name, const glm::mat3 &value) const {
    CHAT_GL_CHECK(glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value)));
}

void GlShaderProgram::setUniform(not_null<const char *> name, const glm::vec4 &value) const {
    CHAT_GL_CHECK(glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value)));
}

void GlShaderProgramsManager::init() {
    const char *FRAG_TEXTURE_ARGB8888_SHADER_SRC = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        
        in vec2 TexCoord;
        
        uniform sampler2D texture_;
        
        void main()
        {
            vec4 argb_col = texture(texture_, TexCoord);
            /*argb_col = vec4(0.4);*/
        	FragColor = argb_col.rgba;
        }
    )GLSL";

    const char *FRAG_COLOR_SHADER_SRC = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec4 color;
        
        void main()
        {
             FragColor = color;
        }
    )GLSL";

    const char *VERT_TEXTURE_SHADER_SRC = R"GLSL(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        
        out vec2 TexCoord;
    
        uniform mat3 transform;
        
        void main()
        {
            /*gl_Position = vec4(transform * aPos, 1.0);*/
            gl_Position = vec4(aPos, 1.0);
        	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
        }
    )GLSL";

    const char *VERT_COLOR_SHADER_SRC = R"GLSL(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat3 transform;
        
        void main()
        {
            /*gl_Position = vec4(transform * aPos, 1.0);*/
            gl_Position = vec4(aPos, 1.0);
        }
    )GLSL";

    colored = GlShaderProgram(VERT_COLOR_SHADER_SRC, FRAG_COLOR_SHADER_SRC);
    textured_argb8888 = GlShaderProgram(VERT_TEXTURE_SHADER_SRC, FRAG_TEXTURE_ARGB8888_SHADER_SRC);
}

void GlRectPolygon::genVertices(const RectPos &pos, uint8_t z, bool add_texture_coords) noexcept {
    std::array<GLfloat, VERTICES_ARRAY_MAX_SIZE> vertices_copy;
    float fz = -static_cast<float>(z) / Z_DEPTH;

    // clang-format off
    if (add_texture_coords)
        vertices_copy = {
            pos.tr.x, pos.tr.y, fz, 1, 1, // top right
            pos.tr.x, pos.bl.y, fz, 1, 0, // bottom right
            pos.bl.x, pos.bl.y, fz, 0, 0, // bottom left
            pos.bl.x, pos.tr.y, fz, 0, 1, // top left
        };
    else
        vertices_copy = {
            pos.tr.x, pos.tr.y, fz, // top right
            pos.tr.x, pos.bl.y, fz, // bottom right
            pos.bl.x, pos.bl.y, fz, // bottom left
            pos.bl.x, pos.tr.y, fz, // top left
        };
    // clang-format on

    std::copy(vertices_copy.begin(), vertices_copy.end(), vertices.begin());
}

GlRectPolygon::GlRectPolygon(const RectPos &pos, uint8_t z, bool add_texture_coords) {
    genVertices(pos, z, add_texture_coords);

    CHAT_GL_CHECK(glGenVertexArrays(1, &vao.getIdRef()));
    CHAT_GL_CHECK(glGenBuffers(1, &vbo.getIdRef()));
    CHAT_GL_CHECK(glGenBuffers(1, &ebo.getIdRef()));

    CHAT_GL_CHECK(glBindVertexArray(vao));

    size_t vertices_arr_size =
        add_texture_coords ? TEX_VERTICES_ARRAY_SIZE : NOTEX_VERTICES_ARRAY_SIZE;

    CHAT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    CHAT_GL_CHECK(glBufferData(
        GL_ARRAY_BUFFER,
        vertices_arr_size * sizeof(vertices[0]),
        vertices.data(),
        GL_DYNAMIC_DRAW
    ));

    CHAT_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    CHAT_GL_CHECK(
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES.begin(), GL_DYNAMIC_DRAW)
    );

    if (add_texture_coords) {
        CHAT_GL_CHECK(glVertexAttribPointer(
            0,
            POS_COORDS_NUM,
            GL_FLOAT,
            GL_FALSE,
            (POS_COORDS_NUM + TEX_COORDS_NUM) * sizeof(GLfloat),
            0
        ));
        CHAT_GL_CHECK(glEnableVertexAttribArray(0));

        CHAT_GL_CHECK(glVertexAttribPointer(
            1,
            TEX_COORDS_NUM,
            GL_FLOAT,
            GL_FALSE,
            (POS_COORDS_NUM + TEX_COORDS_NUM) * sizeof(GLfloat),
            reinterpret_cast<const void *>(POS_COORDS_NUM * sizeof(GLfloat))
        ));
        CHAT_GL_CHECK(glEnableVertexAttribArray(1));
    } else {
        CHAT_GL_CHECK(glVertexAttribPointer(
            0,
            POS_COORDS_NUM,
            GL_FLOAT,
            GL_FALSE,
            POS_COORDS_NUM * sizeof(GLfloat),
            0
        ));
        CHAT_GL_CHECK(glEnableVertexAttribArray(0));
    }
}

void GlRectPolygon::draw() const {
    CHAT_GL_CHECK(glBindVertexArray(vao));
    CHAT_GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
}

void GlRectPolygon::setPosition(const RectPos &new_pos, uint8_t z, bool add_texture_coords) {
    genVertices(new_pos, z, add_texture_coords);

    size_t vertices_arr_size =
        add_texture_coords ? TEX_VERTICES_ARRAY_SIZE : NOTEX_VERTICES_ARRAY_SIZE;

    CHAT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    CHAT_GL_CHECK(glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(vertices_arr_size * sizeof(GLfloat)),
        vertices.data()
    ));
}

void GlTexture::flush() {
    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, obj));
    CHAT_GL_CHECK(
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, res.x, res.y, GL_RGBA, GL_UNSIGNED_BYTE, buf)
    );
}

bool GlTexture::waitForSync(std::chrono::nanoseconds timeout) {
    // TODO: add streaming texture.
    return true;
};

void GlTexture::prepareShader(const GlShaderProgramsManager &shp_man) const {
    shp_man.getTextured().use();
    shp_man.getTextured().setUniform("texture_", 0);

    CHAT_GL_CHECK(glActiveTexture(GL_TEXTURE0));
    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, obj));
}

GlTexture::GlTexture(Vec2I res_, TextureMode mode_)
    : res(res_),
      mode(mode_) {
    if (mode == TextureMode::NO_TEXTURE)
        return;

    CHAT_GL_CHECK(glGenTextures(1, &obj.getIdRef()));

    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, obj));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    buf_capacity = static_cast<size_t>(res.x * res.y) * sizeof(unsigned char) * TEXTURE_CHANNELS;
    buf = HeapArray<unsigned char>(buf_capacity);

    CHAT_GL_CHECK(
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res.x, res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf)
    );
}

void GlTexture::resize(Vec2I new_res) {
    res = new_res;

    size_t new_buf_size = static_cast<size_t>(new_res.x * new_res.y) * TEXTURE_CHANNELS;
    if (new_buf_size > buf_capacity) {
        buf_capacity = new_buf_size;
        buf = HeapArray<unsigned char>(new_buf_size);
    }

    CHAT_GL_CHECK(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        new_res.x,
        new_res.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        buf
    ));
}

GlDrawableRect::GlDrawableRect(
    std::reference_wrapper<GlRenderer> renderer_,
    const DrawableRectConfig &config
)
    : renderer(renderer_),
      texture(config.texture_res, config.texture_mode),
      polygon({config.pos, config.z, texture.isEnabled()}),
      color(colorToF(config.color)) {}

void GlDrawableRect::draw(const GlShaderProgramsManager &shp_man) const {
    if (texture.isEnabled()) {
        texture.prepareShader(shp_man);
    } else {
        shp_man.getColored().use();
        shp_man.getColored().setUniform("color", color);
    }

    polygon.draw();
}

void GlRenderer::handleGlewError(GLenum err) {
    throw GraphicsException(str(format("GLEW error: %1%") % glewGetErrorString(err)));
}

void GlRenderer::onGlErrorCallback(
    GLenum source,
    GLenum type,
    [[maybe_unused]] GLuint id,
    GLenum severity,
    [[maybe_unused]] GLsizei length,
    const GLchar *message,
    [[maybe_unused]] const void *user_param
) {
    const GlRenderer *this_ = static_cast<const GlRenderer *>(user_param);

    Logger::Severity logger_severity;
    const char *severity_str;

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        logger_severity = Logger::ERROR;
        severity_str = "HIGH";
        break;

    case GL_DEBUG_SEVERITY_MEDIUM:
        logger_severity = Logger::WARNING;
        severity_str = "MEDIUM";
        break;

    case GL_DEBUG_SEVERITY_LOW:
        logger_severity = Logger::WARNING;
        severity_str = "LOW";
        break;

    case GL_DEBUG_SEVERITY_NOTIFICATION:
        logger_severity = Logger::VERBOSE;
        severity_str = "NOTE";
        break;

    default:
        logger_severity = Logger::ERROR;
        severity_str = "UNKNOWN";
        break;
    }

#define CHAT_GL_CASE_STR(str_var, value) \
    case value:                          \
        (str_var) = #value;              \
        break;

    const char *source_str;
    switch (source) {
        CHAT_GL_CASE_STR(source_str, GL_DEBUG_SOURCE_API);
        CHAT_GL_CASE_STR(source_str, GL_DEBUG_SOURCE_WINDOW_SYSTEM);
        CHAT_GL_CASE_STR(source_str, GL_DEBUG_SOURCE_SHADER_COMPILER);
        CHAT_GL_CASE_STR(source_str, GL_DEBUG_SOURCE_THIRD_PARTY);
        CHAT_GL_CASE_STR(source_str, GL_DEBUG_SOURCE_APPLICATION);
        CHAT_GL_CASE_STR(source_str, GL_DEBUG_SOURCE_OTHER);
    default:
        source_str = "UNKNOWN";
    }

    const char *type_str;
    switch (type) {
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_ERROR);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_PORTABILITY);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_PERFORMANCE);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_MARKER);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_PUSH_GROUP);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_POP_GROUP);
        CHAT_GL_CASE_STR(type_str, GL_DEBUG_TYPE_OTHER);
    default:
        type_str = "UNKNOWN";
    }
#undef CHAT_GL_CASE_STR

    CHAT_LOG_FILE_LINE(
        logger_severity,
        nullptr,
        0,
        format("GL renderer: %p, %s, %s(%x), %s(%x): %s") % this_ % severity_str % source_str %
            source % type_str % type % message
    );
}

GlRenderer::GlRenderer(Color clear_color_, bool enable_debug_log, bool enable_blur_)
    : clear_color(clear_color_),
      enable_blur(enable_blur_) {

    GLenum err = glewInit();
    if (err != GLEW_OK)
        handleGlewError(err);

    GLint gl_major_ver, gl_minor_ver;
    glGetIntegerv(GL_MAJOR_VERSION, &gl_major_ver);
    glGetIntegerv(GL_MINOR_VERSION, &gl_minor_ver);

    CHAT_LOGI(format("GL version %1%.%2%") % gl_major_ver % gl_minor_ver);

    if (enable_debug_log) {
        if (gl_major_ver >= 4 && gl_minor_ver >= 3) {
            try {
                CHAT_GL_CHECK(glEnable(GL_DEBUG_OUTPUT));
                CHAT_GL_CHECK(glDebugMessageCallback(onGlErrorCallback, this));
            } catch (const GraphicsException &e) {
                CHAT_LOGE(format("failed to enable GL debug log %1%") % e.what());
            }
        } else {
            CHAT_LOGE("GL version is lower than 4.3, debug log is unavailable");
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef CHAT_GL_WIREFRAME
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif // CHAT_GL_WIREFRAME

    shader_programs_manager.init();
}

not_null<shared_ptr<DrawableRect>> GlRenderer::createRect(const DrawableRectConfig &conf) {
    auto ptr = std::make_shared<GlDrawableRect>(*this, conf);

    /*if (conf.bg_blur_radius != 0 && enable_blur)*/
    /*    blurred_rects.emplace_back(ptr);*/
    /*else*/

    rects.emplace_back(ptr);

    return ptr;
}

// shared_ptr<DrawableRect>
// GlRenderer::createTexturedRect(const RectF &pos, float z, const Vec2I res) {
//     shared_ptr<DrawableRect> ptr =
//         std::make_shared<GlDrawableRect>(pos, *textured_shader_prog, ColorF{}, res, 0);
//
//     rects.emplace_back(ptr);
//
//     return ptr;
// }
//
// shared_ptr<DrawableRect>
// GlRenderer::createColoredRect(const RectF &pos, const Color &color, float z) {
//     shared_ptr<DrawableRect> ptr =
//         std::make_shared<GlDrawableRect>(pos, *colored_shader_prog, colorToF(color), Vec2I{}, z);
//
//     rects.emplace_back(ptr);
//
//     return ptr;
// }

void GlRenderer::resize(Vec2I frame_buf_size) {
    CHAT_GL_CHECK(glViewport(0, 0, frame_buf_size.x, frame_buf_size.y));
}

void GlRenderer::draw() const {
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto it = rects.begin(); it != rects.end();) {
        if (auto rect = it->lock()) {
            rect->draw(shader_programs_manager);
            ++it;
        } else {
            it = rects.erase(it);
        }
    }
}
