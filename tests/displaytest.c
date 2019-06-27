#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include "display.c"

EGL_STATE_T state, *p_state = &state;
static volatile sig_atomic_t KILLED = 0;

static void sig_handler(int _)
{
    (void)_;
    KILLED = 1;
}

int main(int argc, char **argv)
{
    VGImage                         vg_img;
    EGL_DISPMANX_WINDOW_T           nativewindow;

    init_egl(p_state);
    init_dispmanx(&nativewindow);
    egl_from_dispmanx(p_state, &nativewindow);
    if (eglMakeCurrent(state.display, state.surface,
                       state.surface, state.context) == EGL_FALSE) {
        perror("Failed to eglMakeCurrent");
        exit(EXIT_FAILURE);
    }
    int w = (int)nativewindow.width;
    int h = (int)nativewindow.height;
    vg_img = vgCreateImage(VG_sL_8, w,h, VG_IMAGE_QUALITY_NONANTIALIASED);
    if (vg_img == VG_INVALID_HANDLE) {
        fprintf(stderr, "Can't create vg_img\n");
        fprintf(stderr, "Error code %x\n", vgGetError());
        exit(2);
    }


    printf("Native Window Size: (%d,%d)\n", w, h);
    signal(SIGINT, sig_handler);
    char  display_buf[w*h];
    for (int i = 0; !KILLED; i++) {
        memset(display_buf, i%255, w*h);
        vgImageSubData(vg_img, display_buf, w, VG_sL_8, 0, 0, w, h);
        vgSetPixels(0, 0, vg_img, 0, 0, w, h);
        eglSwapBuffers(p_state->display, p_state->surface);
    }

    vgDestroyImage(vg_img);
    egl_deinit(&state);

    return 0;
}