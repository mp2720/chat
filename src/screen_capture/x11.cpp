#include "x11.h"
#include "bmp.h"
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <cassert>
#include <iostream>

X11ScreenCapturer::X11ScreenCapturer() {
    display = XOpenDisplay(nullptr);
    if (!display) {
        assert(false);
    }

    screen = XDefaultScreen(display);

    shm_info = new XShmSegmentInfo();

    image = XShmCreateImage(
        display,
        DefaultVisual(display, screen),
        DefaultDepth(display, screen),
        ZPixmap,
        NULL,
        shm_info,
        DisplayWidth(display, screen),
        DisplayHeight(display, screen)
    );

    shm_info->shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
    shm_info->readOnly = False;
    shm_info->shmaddr = image->data = (char *)shmat(shm_info->shmid, 0, 0);

    XShmAttach(display, shm_info);
}

void X11ScreenCapturer::process_frame() {
    while (1) {
        int count_screens = ScreenCount(display);
        std::cout << count_screens << '\n';
    }

    if (!XShmGetImage(display, RootWindow(display, screen), image, 0, 0, AllPlanes))
        assert(false);

    bmp::Bitmap img(image->width, image->height);

    for (int x = 0; x < image->width; ++x) {
        for (int y = 0; y < image->height; ++y) {
            char *pixel = &image->data[y * image->bytes_per_line + x * 4];
            img.set(x, y, bmp::Pixel(pixel[2], pixel[1], pixel[0]));
        }
    }

    img.save("image.bmp");
}
