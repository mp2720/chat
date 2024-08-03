#include "window.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>

using namespace chat::gui;

void Window::resize() {
    // simple test
    // very slow

    Vec2i fb_size = system_window->getFrameBufferSize();
    assert(fb_size.x != 0);

    canvas_texture->resize(fb_size);
    renderer->resize(fb_size);

    auto &data = canvas_texture->getTextureData();

    for (int x = 0; x < fb_size.x * fb_size.y * 4; ++x) {
        data[x] = rand();
    }

    canvas_texture->notifyTextureBufChanged();
}

Window::Window(unique_ptr<SystemWindow> system_window_, unique_ptr<Renderer> renderer_)
    : system_window(std::move(system_window_)), renderer(std::move(renderer_)) {

    canvas_texture = renderer->createTexture({-0.5, -0.5}, {0.5, 0.5}, {100, 100});

    resize();
}

void Window::update() {
    can_continue = !system_window->shouldClose();
    if (!can_continue)
        return;

    system_window->switchContext();

    if (system_window->isResizeRequried())
        resize();

    if (system_window->isResizeRequried() || system_window->isRefreshRequried())
        renderer->draw();

    system_window->swapBuffers();

    // need_redraw = ...

    prev_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}
