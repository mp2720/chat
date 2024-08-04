#include "window.hpp"
#include "gui/backends/graphics.hpp"
#include "gui/backends/sys.hpp"
#include "gui/vec.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>

using namespace chat::gui;

using backends::SystemWindow, backends::Renderer, backends::DrawableRect;

static RectF canvas_pos{-0.5, -0.5, 0.5, 0.5};

void Window::resize() {
    // simple test
    // very slow

    Vec2I fb_size = system_window->getFrameBufferSize();
    assert(fb_size.x != 0);

    /*canvas_texture->resize(fb_size);*/
    renderer->resize(fb_size);

    /*auto &data = canvas_texture->getTextureData();*/
    /**/
    /*for (int x = 0; x < fb_size.x * fb_size.y * 4; ++x) {*/
    /*    data[x] = rand();*/
    /*}*/

    /*canvas_texture->notifyTextureBufChanged();*/
}

Window::Window(unique_ptr<SystemWindow> system_window_, unique_ptr<Renderer> renderer_)
    : system_window(std::move(system_window_)), renderer(std::move(renderer_)) {

    texture1 = renderer->createColoredRect(canvas_pos, {128, 0, 128, 255}, -0.4);
    texture2 = renderer->createColoredRect({-0.3, -0.3, 0.8, 0.8}, {0, 128, 128, 100}, -1.);

    /*canvas_texture = renderer->createTexture({-0.5, -0.5}, {0.5, 0.5}, {100, 100});*/

    resize();
}

void Window::update() {
    can_continue = !system_window->shouldClose();
    if (!can_continue)
        return;

    system_window->switchContext();

    if (system_window->isResizeRequried())
        resize();

    if (system_window->isResizeRequried() || system_window->isRefreshRequried()) {
        canvas_pos.x += 0.001;
        /*texture1->setPosition(canvas_pos);*/
        renderer->draw();
    }

    system_window->swapBuffers();

    system_window->update();

    // need_redraw = ...

    prev_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}
