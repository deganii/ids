#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include "display.h"
#include "bcm_host.h"


void init_display(display_state *state){

    init_egl(&(state->egl_state));
    init_dispmanx(&(state->nativewindow));
    egl_from_dispmanx(&(state->egl_state), &(state->nativewindow));
    if (eglMakeCurrent(state->egl_state.display, state->egl_state.surface,
                       state->egl_state.surface, state->egl_state.context) == EGL_FALSE) {
        perror("Failed to eglMakeCurrent");
        exit(EXIT_FAILURE);
    }
}

void deinit_display(display_state *state){
    egl_deinit(&(state->egl_state));
}

void init_egl(egl_state *state)
{
    EGLint num_configs;
    EGLBoolean result;

    static const EGLint attribute_list[] =
	{
	    EGL_RED_SIZE, 5,
	    EGL_GREEN_SIZE, 6,
	    EGL_BLUE_SIZE, 5,
	    EGL_ALPHA_SIZE, 0,
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	    EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT,
	    EGL_NONE
	};

    // get an EGL display connection
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // initialize the EGL display connection
    result = eglInitialize(state->display, NULL, NULL);

    // get an appropriate EGL frame cam_buffer configuration
    result = eglChooseConfig(state->display, attribute_list, &state->config, 1, &num_configs);
    assert(EGL_FALSE != result);

    result = eglBindAPI(EGL_OPENVG_API);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    state->context = eglCreateContext(state->display,
				      state->config, EGL_NO_CONTEXT,
				      NULL);
    assert(state->context!=EGL_NO_CONTEXT);
}

void init_dispmanx(EGL_DISPMANX_WINDOW_T *nativewindow) {
    int32_t success = 0;
    uint32_t screen_width;
    uint32_t screen_height;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    bcm_host_init();

    // create an EGL window surface
    success = graphics_get_display_size(0 /* LCD */,
					&screen_width,
					&screen_height);
    assert( success >= 0 );

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screen_width;
    dst_rect.height = screen_height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = screen_width << 16;
    src_rect.height = screen_height << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element =
	vc_dispmanx_element_add(dispman_update, dispman_display,
				0/*layer*/, &dst_rect, 0/*src*/,
				&src_rect, DISPMANX_PROTECTION_NONE,
				0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

    // Build an EGL_DISPMANX_WINDOW_T from the Dispmanx window
    nativewindow->element = dispman_element;
    nativewindow->width = screen_width;
    nativewindow->height = screen_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    printf("Got a Dispmanx window\n");
}

void egl_from_dispmanx(egl_state *state,
		       EGL_DISPMANX_WINDOW_T *nativewindow) {
    EGLBoolean result;

    state->surface = eglCreateWindowSurface(state->display,
					    state->config,
					    nativewindow, NULL );
    assert(state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);

    // add features needed by shape.c
    // we are always a fullscreen window at (0,0)
    state->window_x = 0;
	state->window_y = 0;
	state->window_width = nativewindow->width;
	state->window_height = nativewindow->height;
}

void egl_deinit(egl_state *state)
{
	// free the egsImage vgDestroyPaint
	eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	assert(eglGetError() == EGL_SUCCESS);
	eglTerminate(state->display);
	assert(eglGetError() == EGL_SUCCESS);
	eglReleaseThread();
}
