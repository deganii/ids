//
// Created by idegani on 7/12/2019.
//

#ifndef IDS_EGL_RENDER_H
#define IDS_EGL_RENDER_H
#include "display.h"
#include "app.h"
#ifdef __cplusplus
extern "C" {
#endif

int egl_component_init(display_state *disp_state);
void egl_render_buffer(void *rgbBuffer, int len);
void egl_component_deinit(display_state *disp_state);


#ifdef __cplusplus
}
#endif

#endif //IDS_EGL_RENDER_H
