#include "graphics_gl_impl.hpp"

#include "../gl_include.h"
#include "gui/vec.hpp"
#include "log.hpp"
#include "ptr.hpp"
#include <array>
#include <boost/format.hpp>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

using namespace chat;
using namespace gui;
using namespace backends;
using namespace gl_details;

using boost::str, boost::format;
using chat::Logger;

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

    GlScalarObject obj{glDeleteProgram};

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

        throw GraphicsException(str(format("failed to link shader program: %2%") % info_log));
    }
}

void GlShaderProgram::use() {
    CHAT_GL_CHECK(glUseProgram(obj));
}

void GlShaderProgram::setUniform(not_null<const char *> name, int value) {
    CHAT_GL_CHECK(glUniform1i(getUniformLocation(name), (int)value));
}

void GlShaderProgram::setUniform(not_null<const char *> name, float value) {
    CHAT_GL_CHECK(glUniform1f(getUniformLocation(name), value));
}

void GlShaderProgram::setUniform(not_null<const char *> name, const glm::mat3 &value) {
    CHAT_GL_CHECK(glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value)));
}

void GlShaderProgram::setUniform(not_null<const char *> name, const glm::vec4 &value) {
    CHAT_GL_CHECK(glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value)));
}

constexpr static const char *const FRAG_TEXTURE_SHADER_SRC = R"GLSL(
    #version 330 core
    out vec4 FragColor;
    
    in vec2 TexCoord;
    
    uniform sampler2D texture_;
    
    void main()
    {
    	 FragColor = texture(tex, TexCoord);
    }
)GLSL";

constexpr static const char *const FRAG_COLOR_SHADER_SRC = R"GLSL(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec4 color;
    
    void main()
    {
    	 vec4 col1 = color;
         /*col1.a = 0.2;*/
         FragColor = col1;
    }
)GLSL";

constexpr static const char *const VERT_TEXTURE_SHADER_SRC = R"GLSL(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord;

    uniform mat3 transform;
    
    void main()
    {
        gl_Position = vec4(transform * aPos, 1.0);
    	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    }
)GLSL";

constexpr static const char *const VERT_COLOR_SHADER_SRC = R"GLSL(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    
    /*uniform mat3 transform;*/
    
    void main()
    {
        /*gl_Position = vec4(transform * aPos, 1.0);*/
        gl_Position = vec4(aPos, 1.0);
    }
)GLSL";

size_t GlDrawableRect::genVertices(const RectF &pos, bool is_textured, float z) {
    std::array<GLfloat, VERTICES_ARRAY_MAX_SIZE> vertices_copy;
    size_t vertices_array_size;
    if (is_textured) {
        // clang-format off
        vertices_copy = {
            //  positions      texture coords
            //  x    y    z         x  y
            pos.z, pos.w, 0,        1, 1,       // top right
            pos.z, pos.y, 0,        1, 0,       // bottom right
            pos.x, pos.y, 0,        0, 0,       // bottom left
            pos.x, pos.w, 0,        0, 1,       // top left
        };
        // clang-format on 
        
        vertices_array_size = VERTICES * (3 + 2);
    } else {
        // clang-format off
        vertices_copy = {
            //  positions
            //  x    y    z 
            pos.z, pos.w, z, // top right
            pos.z, pos.y, z, // bottom right
            pos.x, pos.y, z, // bottom left
            pos.x, pos.w, z, // top left
        };
        // clang-format on
        //
        vertices_array_size = VERTICES * 3;
    }
    std::copy(vertices_copy.begin(), vertices_copy.end(), vertices.begin());
    return vertices_array_size;
}

GlDrawableRect::GlDrawableRect(
    RectF pos,
    GlShaderProgram &shader_program_,
    ColorF color_,
    Vec2I texture_res_,
    float z
)
    : shader_program(shader_program_), color(color_), texture_res(texture_res_) {

    bool is_textured = texture_res != Vec2I{0, 0};

    CHAT_GL_CHECK(glGenVertexArrays(1, &vao.id()));
    CHAT_GL_CHECK(glGenBuffers(1, &vbo.id()));
    CHAT_GL_CHECK(glGenBuffers(1, &ebo.id()));

    CHAT_GL_CHECK(glBindVertexArray(vao));

    size_t vertices_array_size = genVertices(pos, is_textured, z);

    CHAT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    CHAT_GL_CHECK(glBufferData(
        GL_ARRAY_BUFFER,
        vertices_array_size * sizeof(vertices[0]),
        vertices.data(),
        GL_STATIC_DRAW
    ));

    CHAT_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    CHAT_GL_CHECK(
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(INDICES), INDICES.begin(), GL_STATIC_DRAW)
    );

    /*shader_program.use();*/

    if (is_textured) {
        // positions
        CHAT_GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0));
        CHAT_GL_CHECK(glEnableVertexAttribArray(0));
        // texture coords
        CHAT_GL_CHECK(glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(GLfloat),
            reinterpret_cast<const void *>(3 * sizeof(GLfloat))
        ));
        CHAT_GL_CHECK(glEnableVertexAttribArray(1));

        CHAT_GL_CHECK(glGenTextures(1, &texture.id()));

        CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
        CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

        CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

        shader_program.setUniform("texture_", 0);

        texture_size = static_cast<size_t>(texture_res.x * texture_res.y) * TEXTURE_CHANNELS;
        texture_data = HeapArray<unsigned char>(texture_size);

        CHAT_GL_CHECK(glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            texture_res.x,
            texture_res.y,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            texture_data
        ));
    } else {
        // positions
        CHAT_GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0));
        CHAT_GL_CHECK(glEnableVertexAttribArray(0));

        /*shader_program.setUniform("color", color);*/
    }

    /*shader_program.setUniform("transform", transform_mat);*/
}

void GlDrawableRect::draw() {
    if (isTextured()) {
        CHAT_GL_CHECK(glActiveTexture(GL_TEXTURE0));
        CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    }

    shader_program.use();

    shader_program.setUniform("color", color);

    CHAT_GL_CHECK(glBindVertexArray(vao));
    CHAT_GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
}

void GlDrawableRect::setPosition(const RectF &new_pos) {
    size_t vertices_array_size = genVertices(new_pos, isTextured(), 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(vertices_array_size * sizeof(GLfloat)),
        vertices.data()
    );
}

void GlDrawableRect::resizeTexture(Vec2I new_res) {
    checkIsTextured();

    size_t new_size = static_cast<size_t>(new_res.x * new_res.y) * TEXTURE_CHANNELS;
    if (new_size <= texture_size)
        return;

    texture_size = new_size;
    texture_res = new_res;
    texture_data = HeapArray<unsigned char>(new_size);

    CHAT_GL_CHECK(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        new_res.x,
        new_res.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture_data
    ));
}

void GlDrawableRect::flushTexture() {
    checkIsTextured();

    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture));
    CHAT_GL_CHECK(glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        texture_res.x,
        texture_res.y,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture_data
    ));
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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /*glEnable(GL_DEPTH_TEST);*/
    /*glEnable(GL_BLEND);*/
    /*glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

    colored_shader_prog = new GlShaderProgram(VERT_COLOR_SHADER_SRC, FRAG_COLOR_SHADER_SRC);
}

shared_ptr<DrawableRect> GlRenderer::createTexturedRect(const RectF &pos, float z, const Vec2I res) {
    shared_ptr<DrawableRect> ptr =
        std::make_shared<GlDrawableRect>(pos, *textured_shader_prog, ColorF{}, res, 0);

    rects.emplace_back(ptr);

    return ptr;
}

shared_ptr<DrawableRect>
GlRenderer::createColoredRect(const RectF &pos, const Color &color, float z) {
    shared_ptr<DrawableRect> ptr =
        std::make_shared<GlDrawableRect>(pos, *colored_shader_prog, colorToF(color), Vec2I{}, z);

    rects.emplace_back(ptr);

    return ptr;
}

void GlRenderer::resize(Vec2I frame_buf_size) {
    CHAT_GL_CHECK(glViewport(0, 0, frame_buf_size.x, frame_buf_size.y));
}

void GlRenderer::draw() {
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto it = rects.begin(); it != rects.end();) {
        if (auto rect = it->lock()) {
            rect->draw();
            ++it;
        } else {
            it = rects.erase(it);
        }
    }
}
