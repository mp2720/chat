#include "graphics_gl_impl.hpp"

#include "../gl_include.h"
#include "log.hpp"
#include "ptr.hpp"
#include <boost/format.hpp>
#include <memory>

using namespace chat::gui;

using boost::str, boost::format;
using chat::Logger;
using chat::shared_ptr;

#ifdef CHAT_GL_IGNORE_ERRORS
#define CHAT_GL_CHECK(stmt) stmt
#else
static void checkGlError(const char *stmt) {
    GLenum err = glGetError();
    if (err == GL_NO_ERROR)
        return;

    // Зачем возвращать const unsigned char * ???
    const char *str = reinterpret_cast<const char *>(glewGetErrorString(err));
    throw GraphicsException(::str(format("GL error %1% in %2%") % str % stmt));
}

#define CHAT_GL_CHECK(stmt)  \
    do {                     \
        stmt;                \
        checkGlError(#stmt); \
    } while (0)
#endif

GlObject &GlObject::operator=(GlObject &&other) noexcept {
    if (this != &other) {
        this->~GlObject();
        id_ = other.id_;
        other.id_ = 0;
    }
    return *this;
}

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

    CHAT_GL_CHECK(obj.id() = glCreateShader(gl_type));
    if (obj.id() == 0)
        throw GraphicsException("failed to create shader");

    const char *src_ = src;

    CHAT_GL_CHECK(glShaderSource(obj, 1, &src_, nullptr));
    CHAT_GL_CHECK(glCompileShader(obj));

    int ok;
    CHAT_GL_CHECK(glGetShaderiv(obj, GL_COMPILE_STATUS, &ok));
    if (!ok) {
        char info_log[INFO_LOG_SIZE];
        CHAT_GL_CHECK(glGetShaderInfoLog(obj, sizeof info_log, nullptr, info_log));

        this->~Shader();

        throw GraphicsException(
            str(format("failed to compile %1% shader: %2%") % type_str % info_log)
        );
    }
}

GLint GlShaderProgram::getUniformLocation(not_null<const char *> name) const {
    GLint loc;
    CHAT_GL_CHECK(loc = glGetUniformLocation(obj, name));
    if (loc < 0)
        throw GraphicsException(str(format("uniform %1% not found") % name));

    return loc;
}

GlShaderProgram::GlShaderProgram(
    not_null<const char *> vertex_src,
    not_null<const char *> frag_src
) {
    Shader vertex(vertex_src, Shader::Type::VERTEX), fragment(frag_src, Shader::Type::FRAGMENT);

    CHAT_GL_CHECK(obj.id() = glCreateProgram());
    if (obj.id() == 0)
        throw new GraphicsException("failed to create shader program");

    CHAT_GL_CHECK(glAttachShader(obj, vertex.getId()));
    CHAT_GL_CHECK(glAttachShader(obj, fragment.getId()));
    CHAT_GL_CHECK(glLinkProgram(obj));

    int ok;
    CHAT_GL_CHECK(glGetProgramiv(obj, GL_LINK_STATUS, &ok));
    if (!ok) {
        char info_log[INFO_LOG_SIZE];
        CHAT_GL_CHECK(glGetProgramInfoLog(obj, sizeof info_log, nullptr, info_log));

        this->~GlShaderProgram();

        throw GraphicsException(str(format("failed to link shader program: %2%") % info_log));
    }
}

void GlShaderProgram::use() const {
    CHAT_GL_CHECK(glUseProgram(obj));
}

void GlShaderProgram::setBool(not_null<const char *> name, bool value) const {
    CHAT_GL_CHECK(glUniform1i(getUniformLocation(name), (int)value));
}

void GlShaderProgram::setInt(not_null<const char *> name, int value) const {
    CHAT_GL_CHECK(glUniform1i(getUniformLocation(name), (int)value));
}

void GlShaderProgram::setFloat(not_null<const char *> name, float value) const {
    CHAT_GL_CHECK(glUniform1f(getUniformLocation(name), value));
}

GlTexturedRect::GlTexturedRect(Vec2f bl_pos, Vec2f tr_pos, Vec2i size_)
    : shader_program(GlShaderProgram(VERT_SHADER_SRC, FRAG_SHADER_SRC)), size(size_) {

    CHAT_GL_CHECK(glGenVertexArrays(1, &vao.id()));
    CHAT_GL_CHECK(glGenBuffers(1, &vbo.id()));
    CHAT_GL_CHECK(glGenBuffers(1, &ebo.id()));

    CHAT_GL_CHECK(glBindVertexArray(vao));

    std::array<float, 20> vertices_copy = {
        //    positions       | texture coords
        //  x        y      z |x  y
        tr_pos.x, tr_pos.y, 0, 1, 1, // top right
        tr_pos.x, bl_pos.y, 0, 1, 0, // bottom right
        bl_pos.x, bl_pos.y, 0, 0, 0, // bottom left
        bl_pos.x, tr_pos.y, 0, 0, 1  // top left
    };
    std::copy(vertices_copy.begin(), vertices_copy.end(), vertices.begin());

    CHAT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    CHAT_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW));

    CHAT_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    CHAT_GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES, GL_STATIC_DRAW));

    // positions
    CHAT_GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0));
    CHAT_GL_CHECK(glEnableVertexAttribArray(0));
    // texture coords
    CHAT_GL_CHECK(glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void *)(3 * sizeof(float))
    ));
    CHAT_GL_CHECK(glEnableVertexAttribArray(1));

    CHAT_GL_CHECK(glGenTextures(1, &obj.id()));

    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, obj));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    resize(size);

    shader_program.use();
    shader_program.setInt("tex", 0);
}

void GlTexturedRect::draw() const {
    CHAT_GL_CHECK(glActiveTexture(GL_TEXTURE0));
    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, obj));

    shader_program.use();
    CHAT_GL_CHECK(glBindVertexArray(vao));
    CHAT_GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
}

void GlTexturedRect::notifyTextureBufChanged() {
    // TODO: fix bug
    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, obj));
    CHAT_GL_CHECK(glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        size.x,
        size.y,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture_buf.data()
    ));
}

void GlTexturedRect::resize(Vec2i size) {
    // TODO: fix bug
    texture_buf.resize(size.x * size.y * CHANNELS);

    CHAT_GL_CHECK(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        size.x,
        size.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        static_cast<const void *>(texture_buf.data())
    ));
    CHAT_GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
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
        format("GL %s, %s(%x), %s(%x): %s") % severity_str % source_str % source % type_str % type %
            message
    );
}

GlRenderer::GlRenderer(Color clear_color_, bool enable_debug_log) : clear_color(clear_color_) {
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
                CHAT_GL_CHECK(glDebugMessageCallback(onGlErrorCallback, nullptr));
            } catch (const GraphicsException &e) {
                CHAT_LOGE(format("failed to enable GL debug log %1%") % e.what());
            }
        } else {
            CHAT_LOGE("GL version is lower than 4.3, debug log is unavailable");
        }
    }
}

shared_ptr<TexturedRect> GlRenderer::createTexture(Vec2f bl_pos, Vec2f tr_pos, Vec2i size) {
    shared_ptr<TexturedRect> ptr = std::make_shared<GlTexturedRect>(bl_pos, tr_pos, size);

    textured_rects.push_back(ptr);

    return ptr;
};

void GlRenderer::resize(Vec2i frame_buf_size) {
    CHAT_GL_CHECK(glViewport(0, 0, frame_buf_size.x, frame_buf_size.y));
}

void GlRenderer::draw() const {
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto it = textured_rects.begin(); it != textured_rects.end();) {
        if (auto rect = it->lock()) {
            rect->draw();
            ++it;
        } else {
            it = textured_rects.erase(it);
        }
    }
}
