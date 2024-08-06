#include "window.hpp"
#include "gui/backends/graphics.hpp"
#include "gui/backends/sys.hpp"
#include "gui/vec.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>

using namespace chat::gui;
using namespace backends;

static RectPos rect1_pos{{-0.5, -0.5}, {0.5, 0.5}};

void Window::resize() {
    Vec2I fb_size = system_window->getFrameBufferSize();
    assert(fb_size.x != 0);

    /*auto texture1 = rect1->requireTexture();*/
    /*texture1->resize(fb_size / 5);*/

    renderer->resize(fb_size);
}

Window::Window(unique_ptr<SystemWindow> system_window_, unique_ptr<Renderer> renderer_)
    : system_window(std::move(system_window_)),
      renderer(std::move(renderer_)) {

    DrawableRectConfig conf1 = {
        .pos = rect1_pos,
        .z = 100,
        .texture_mode = backends::TextureMode::STATIC_TEXTURE,
        .texture_res = {1000, 300},
        .color = {255, 0, 0, 255},
    };

    DrawableRectConfig conf2 = {
        .pos = {{-0.3, -0.3}, {0.8, 0.8}},
        .z = 100,
        .color = {0, 255, 0, 128},
    };

    rect1 = renderer->createRect(conf1);
    rect2 = renderer->createRect(conf2);

    auto texture1 = rect1->requireTexture();

    if (auto buf_lock = texture1->lockBuf()) {
        unsigned char *ptr = buf_lock->get();
        for (size_t x = 0; x < texture1->getPitch() * texture1->getResolution().y * 4; ++x)
            ptr[x] = rand();
    }
}

void Window::update() {
    can_continue = !system_window->shouldClose();
    if (!can_continue)
        return;

    system_window->switchContext();

    if (system_window->isResizeRequried())
        resize();

    if (system_window->isResizeRequried() || system_window->isRefreshRequried()) {
        rect1_pos.bl.x = sin(prev_update_time.count() / 1000.);
        rect1_pos.tr.x = -rect1_pos.bl.x;

        rect1_pos.tr.y = sin(prev_update_time.count() / 1000.);
        rect1_pos.bl.y = -rect1_pos.tr.y;

        rect1->setPosition(rect1_pos);

        renderer->drawStart();
        renderer->drawRect(rect2.get());
        renderer->drawRect(rect1.get());
    }

    system_window->swapBuffers();

    system_window->update();

    // need_redraw = ...

    prev_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}
