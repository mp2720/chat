#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>

class X11ScreenCapturer {
    Display *display;
    XImage *image = nullptr;
    XShmSegmentInfo *shm_info;
    int screen;

  public:
    X11ScreenCapturer();

    void process_frame();
};
