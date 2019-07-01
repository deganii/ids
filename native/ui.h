#include <VG/openvg.h>
#include <VG/vgu.h>
#include "tis_v4l2.h"
#include "types.h"
#include "touch.h"

#ifndef IDS_UI_H
#define IDS_UI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct component {
    char name[20];
    void (*draw)(struct component*);
    void (*input)(struct component*, touch_event*);
    rect bounds;
    void *state; // custom state
    // add more stuff here as needed:
    // int visible;
    // etc
} component;

typedef struct ui_state {
    size_t num_components;
    coordinate cam_offsets;
} ui_state;

void init_ui(ui_state *state, camera_state *cam_state);
void update_ui(ui_state *state);
void handle_touch_event(touch_event *evt);
void draw_roi_selector(VGfloat x, VGfloat y, VGfloat w, VGfloat h, float off_x, float off_y);
void get_local_coord(rect *bound, coordinate *absolute);
int collide(rect *bound, int x, int y);

void draw_roi(component *comp);
void input_roi(component *comp, touch_event *evt);

void draw_circles(component *comp);
void input_circles(component *comp, touch_event *evt);

void deinit_ui(ui_state *state);

#ifdef __cplusplus
}
#endif

#endif
