#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <VG/openvg.h>
#include <VG/vgu.h>

#ifndef IDS_DISPLAY_H
#define IDS_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

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

} egl_state;

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

typedef struct display_state{
    EGL_DISPMANX_WINDOW_T           nativewindow;
    egl_state                       egl_state;
} display_state;


void init_display(display_state *state);
void deinit_display(display_state *state);

void init_egl(egl_state *state);
void egl_deinit(egl_state *state);
void init_dispmanx(EGL_DISPMANX_WINDOW_T *nativewindow);
void egl_from_dispmanx(egl_state *state, EGL_DISPMANX_WINDOW_T *nativewindow);

#ifdef __cplusplus
}
#endif


#endif

