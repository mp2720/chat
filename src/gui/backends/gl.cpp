#include "gl.hpp"

#include "gui/renderer.hpp"
#include "gui/vec.hpp"
#include "log.hpp"
#include "ptr.hpp"
#include <GL/gl.h>
#include <array>
#include <boost/format.hpp>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

using namespace chat::gui::backends;

using boost::str, boost::format;
using chat::Logger, chat::unique_ptr, chat::not_null;
using chat::gui::RendererException, chat::gui::Renderer, chat::gui::RectPos, chat::gui::Vec2Px,
    chat::gui::TextureMode, chat::gui::Color, chat::gui::ColoredRect, chat::gui::TexturedRect;

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
    throw RendererException(::str(format("GL error %1% in %2%") % str % stmt));
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
        throw RendererException("failed to create shader");

    const char *src_ = src;

    CHAT_GL_CHECK(glShaderSource(obj, 1, &src_, nullptr));
    CHAT_GL_CHECK(glCompileShader(obj));

    int ok;
    CHAT_GL_CHECK(glGetShaderiv(obj, GL_COMPILE_STATUS, &ok));
    if (!ok) {
        char info_log[INFO_LOG_SIZE];
        CHAT_GL_CHECK(glGetShaderInfoLog(obj, sizeof info_log, nullptr, info_log));

        throw RendererException(
            str(format("failed to compile %1% shader: %2%") % type_str % info_log)
        );
    }
}

GLint GlShaderProgram::getUniformLocation(not_null<const char *> name) const {
    GLint loc;
    CHAT_GL_CHECK(loc = glGetUniformLocation(obj, name));
    if (loc < 0)
        throw RendererException(str(format("uniform %1% not found") % name));

    return loc;
}

GlShaderProgram::GlShaderProgram(
    not_null<const char *> vertex_src,
    not_null<const char *> frag_src
) {

    Shader vertex(vertex_src, Shader::Type::VERTEX), fragment(frag_src, Shader::Type::FRAGMENT);
    CHAT_GL_CHECK(obj.getIdRef() = glCreateProgram());
    if (obj.getIdRef() == 0)
        throw new RendererException("failed to create shader program");

    CHAT_GL_CHECK(glAttachShader(obj, vertex.getId()));
    CHAT_GL_CHECK(glAttachShader(obj, fragment.getId()));
    CHAT_GL_CHECK(glLinkProgram(obj));

    int ok;
    CHAT_GL_CHECK(glGetProgramiv(obj, GL_LINK_STATUS, &ok));
    if (!ok) {
        char info_log[INFO_LOG_SIZE];
        CHAT_GL_CHECK(glGetProgramInfoLog(obj, sizeof info_log, nullptr, info_log));

        throw RendererException(str(format("failed to link shader program: %2%") % info_log));
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

void GlRenderer::handleGlewError(GLenum err) {
    throw RendererException(str(format("GLEW error: %1%") % glewGetErrorString(err)));
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

GlRenderer::GlRenderer(bool enable_debug_log, bool enable_blur_)
    : enable_blur(enable_blur_) {

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
            } catch (const RendererException &e) {
                CHAT_LOGE(format("failed to enable GL debug log %1%") % e.what());
            }
        } else {
            CHAT_LOGE("GL version is lower than 4.3, debug log is unavailable");
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef CHAT_GL_WIREFRAME
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif // CHAT_GL_WIREFRAME

    shader_progs_manager.init();
}

void GlRenderer::resize(Vec2Px frame_buf_size) {
    size = frame_buf_size;
    CHAT_GL_CHECK(glViewport(0, 0, frame_buf_size.x, frame_buf_size.y));
}

void GlRenderer::clear() const {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

unique_ptr<TexturedRect>
GlRenderer::createBitmapTexturedRect(const RectPos &pos, Vec2Px res, TextureMode mode) {
    return std::make_unique<GlTexturedRect>(*this, pos, res, mode);
};

unique_ptr<ColoredRect>
GlRenderer::createColoredRect(const RectPos &pos, Color color, float background_blur_radius) {
    return std::make_unique<GlColoredRect>(*this, pos, color, background_blur_radius);
};

void GlRectPolygon::genVertices(const GlViewport &viewport, const RectPos &pos) noexcept {
    std::array<GLfloat, VERTICES_ARRAY_MAX_SIZE> vertices_copy;

    auto gl_tl = viewport.pxToGlCoords(pos.top_left);
    auto gl_size = viewport.pxToGlCoords(pos.size);

    glm::vec2 gl_tr = {gl_tl.x + gl_size.x, gl_tl.y};
    glm::vec2 gl_bl = {gl_tl.x, gl_tl.y - gl_size.y};

    // clang-format off
    if (has_texture_coords)
        vertices_copy = {
            gl_tr.x, gl_tr.y, 0, 1, 1, // top right
            gl_tr.x, gl_bl.y, 0, 1, 0, // bottom right
            gl_bl.x, gl_bl.y, 0, 0, 0, // bottom left
            gl_bl.x, gl_tr.y, 0, 0, 1, // top left
        };
    else
        vertices_copy = {
            gl_tr.x, gl_tr.y, 0, // top right
            gl_tr.x, gl_bl.y, 0, // bottom right
            gl_bl.x, gl_bl.y, 0, // bottom left
            gl_bl.x, gl_tr.y, 0, // top left
        };
    // clang-format on

    std::copy(vertices_copy.begin(), vertices_copy.end(), vertices.begin());
}

GlRectPolygon::GlRectPolygon(
    const GlViewport &viewport,
    const RectPos &pos,
    bool add_texture_coords
)
    : has_texture_coords(add_texture_coords) {

    genVertices(viewport, pos);

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

void GlRectPolygon::setPosition(const GlViewport &viewport, const RectPos &new_pos) {
    genVertices(viewport, new_pos);

    size_t vertices_arr_size =
        has_texture_coords ? TEX_VERTICES_ARRAY_SIZE : NOTEX_VERTICES_ARRAY_SIZE;

    CHAT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    CHAT_GL_CHECK(glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(vertices_arr_size * sizeof(GLfloat)),
        vertices.data()
    ));
}

void GlTexturedRect::flush() {
    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texure));
    CHAT_GL_CHECK(
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, res.x, res.y, GL_RGBA, GL_UNSIGNED_BYTE, buf.get())
    );
}

bool GlTexturedRect::waitForSync(std::chrono::nanoseconds timeout) {
    // TODO: add streaming texture.
    return true;
};

GlTexturedRect::GlTexturedRect(
    GlRenderer &renderer_,
    const RectPos &pos,
    Vec2Px res_,
    TextureMode mode_
)
    : renderer(renderer_),
      polygon(GlViewport(renderer), pos, true),
      res(res_),
      mode(mode_) {

    CHAT_GL_CHECK(glGenTextures(1, &texure.getIdRef()));

    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texure));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    CHAT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    buf_capacity = static_cast<size_t>(res.x * res.y) * sizeof(unsigned char) * TEXTURE_CHANNELS;
    buf = unique_ptr<unsigned char[]>(new unsigned char[buf_capacity]);

    CHAT_GL_CHECK(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        res.x,
        res.y,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        buf.get()
    ));
}

void GlTexturedRect::draw() const {
    const auto &shp_man = renderer.getShaderProgramsManager();
    shp_man.getTextured().use();
    shp_man.getTextured().setUniform("texture_", 0);

    CHAT_GL_CHECK(glActiveTexture(GL_TEXTURE0));
    CHAT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texure));

    polygon.draw();
}

void GlTexturedRect::resize(Vec2Px new_res) {
    res = new_res;

    size_t new_buf_size = static_cast<size_t>(new_res.x * new_res.y) * TEXTURE_CHANNELS;
    if (new_buf_size > buf_capacity) {
        buf_capacity = new_buf_size;
        buf = unique_ptr<unsigned char[]>(new unsigned char[new_buf_size]);
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
        buf.get()
    ));
}

void GlColoredRect::draw() const {
    const auto &shp_man = renderer.getShaderProgramsManager();
    shp_man.getColored().use();
    shp_man.getColored().setUniform("color", color);

    polygon.draw();
}

unique_ptr<Renderer> chat::gui::makeGlRenderer(const RendererConfig &config) {
    return std::make_unique<GlRenderer>(config.enable_debug_log, config.enable_blur);
}
