#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include <ilclient.h>

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

typedef struct display_state{
    EGL_DISPMANX_WINDOW_T           nativewindow;
    egl_state                       egl_state;
    VGImage                         vg_img;
    EGLImageKHR                     eglKHRImage;
//    OMX_BUFFERHEADERTYPE            *eglBuffer;
//    COMPONENT_T                     *egl_render;
    ILCLIENT_T                      *client;

} display_state;


//void init_display(display_state *state);
void init_display(display_state *state, int cam_resx, int cam_resy, int create_khr);
void deinit_display(display_state *state);

void init_egl(egl_state *state);
void egl_deinit(egl_state *state);
void init_dispmanx(EGL_DISPMANX_WINDOW_T *nativewindow);
void egl_from_dispmanx(egl_state *state, EGL_DISPMANX_WINDOW_T *nativewindow);

#ifdef __cplusplus
}
#endif


#endif

