#include "gui.hpp"

#include "gui/vec.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>

using namespace chat::gui;

static RectPos rect1_pos{{-0.5, -0.5}, {0.5, 0.5}};

void Gui::resize() {
    Vec2I fb_size = system_ctx->getFrameBufferSize();
    assert(fb_size.x != 0);

    renderer_ctx->resize(fb_size);
}

Gui::Gui(unique_ptr<Window> system_ctx_, unique_ptr<RendererContext> renderer_ctx)
    : system_ctx(std::move(system_ctx_)),
      renderer_ctx(std::move(renderer_ctx)) {

    DrawableRectConfig conf1 = {
        .pos = rect1_pos,
        .texture_mode = TextureMode::STATIC_TEXTURE,
        .texture_res = {1000, 300},
        .color = {255, 0, 0, 255},
    };

    DrawableRectConfig conf2 = {
        .pos = {{-0.3, -0.3}, {0.8, 0.8}},
        .color = {0, 255, 0, 128},
    };

    rect1 = renderer_ctx->createRect(conf1);
    rect2 = renderer_ctx->createRect(conf2);

    auto texture1 = rect1->requireTexture();

    if (auto buf_lock = texture1->lockBuf()) {
        unsigned char *ptr = buf_lock->get();
        for (size_t x = 0; x < texture1->getPitch() * texture1->getResolution().y * 4; ++x)
            ptr[x] = rand();
    }
}

void Gui::update() {
    can_continue = !system_ctx->shouldClose();
    if (!can_continue)
        return;

    if (system_ctx->isResizeRequried())
        resize();

    if (system_ctx->isResizeRequried() || system_ctx->isRefreshRequried()) {
        rect1_pos.bl.x = sin(prev_update_time.count() / 1000.);
        rect1_pos.tr.x = -rect1_pos.bl.x;

        rect1_pos.tr.y = sin(prev_update_time.count() / 1000.);
        rect1_pos.bl.y = -rect1_pos.tr.y;

        rect1->setPosition(rect1_pos);

        renderer_ctx->drawStart();
        rect2->draw();
        rect1->draw();
    }

    system_ctx->swapBuffers();

    system_ctx->update();

    // need_redraw = ...

    prev_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
}
