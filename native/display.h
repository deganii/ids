#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <VG/openvg.h>
#include <VG/vgu.h>

#ifndef IDS_DISPLAY_H
#define IDS_DISPLAY_H

typedef struct
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;

	uint32_t screen_width;
	uint32_t screen_height;
	int32_t window_x;
	int32_t window_y;
	uint32_t window_width;
	uint32_t window_height;

} EGL_STATE_T;

struct egl_manager {
    EGLNativeDisplayType xdpy;
    EGLNativeWindowType xwin;
    EGLNativePixmapType xpix;

    EGLDisplay dpy;
    EGLConfig conf;
    EGLContext ctx;

    EGLSurface win;
    EGLSurface pix;
    EGLSurface pbuf;
    EGLImageKHR image;

    EGLBoolean verbose;
    EGLint major, minor;
};

void init_egl(EGL_STATE_T *state);
void egl_deinit(EGL_STATE_T *state);
void init_dispmanx(EGL_DISPMANX_WINDOW_T *nativewindow);
void egl_from_dispmanx(EGL_STATE_T *state, EGL_DISPMANX_WINDOW_T *nativewindow);

#endif