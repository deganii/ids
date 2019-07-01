#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "ui.h"
#include "shapes.h"
#include "types.h"
#include "touch.h"

#define SENSOR_FULL_WIDTH  3872.
#define SENSOR_FULL_HEIGHT 2764.
#define CROP_WIDTH  2*1280. //640.
#define CROP_HEIGHT 2*760. //480.

// very simple UI framework:
// draw_xxx will draw a component
// handle_xxx handles a touch on that component



// components in draw-order
component components[] = {
        {"roi_selector", draw_roi, input_roi, {100, 100, 200, 200}},
        {"circle_test", draw_circles, input_circles, {300, 100, 300, 300}}
};

void draw_image_button(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGImage image) {

}

VGfloat get_aspect_scaling(rect *bounds){
    VGfloat aspect = bounds->w / bounds->h;
    VGfloat asp_scale;
    if(aspect > 1.4) { // limiting factor is height
        asp_scale = bounds->h / SENSOR_FULL_HEIGHT;
    }  else {
        asp_scale = bounds->w / SENSOR_FULL_WIDTH;
    }
    return asp_scale;
}

void draw_roi(component *comp){
    rect *bounds = &(comp->bounds);
    coordinate *cam_offset = (coordinate *)comp->state;
    VGfloat asp_scale = get_aspect_scaling(&(comp->bounds));

    Fill(0, 0, 255, 1.0);
    Rect(bounds->x, bounds->y,
        asp_scale * SENSOR_FULL_WIDTH,
        asp_scale * SENSOR_FULL_HEIGHT);

    Fill(255, 0, 0, 1.0);
    Rect(bounds->x+cam_offset->x*asp_scale, bounds->y+asp_scale*cam_offset->y,
         asp_scale * CROP_WIDTH,asp_scale * CROP_HEIGHT);
}

void input_roi(component *comp, touch_event *evt){
    if(evt->event_type == TOUCH_UP) {
        coordinate *cam_offset = (coordinate *) comp->state;
        coordinate new_cam_offset = {evt->x, evt->y};
        get_local_coord(&(comp->bounds), &new_cam_offset);

        VGfloat asp_scale = get_aspect_scaling(&(comp->bounds));

        cam_offset->x = (int) (((float) new_cam_offset.x) / asp_scale);
        cam_offset->y = (int) (((float) new_cam_offset.y) / asp_scale);


        // TODO: change cam position
        fprintf(stderr, "New Camera Offset: (%03d,%03d)", cam_offset->x, cam_offset->y);
    }
}

void draw_circles(component *comp){
    if(comp->state != NULL) {
        coordinate *fingers = (coordinate *) comp->state;
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (fingers[i].x > 0 && fingers[i].y > 0){
                Circle(fingers[i].x,fingers[i].y,25);
            }
        }
    }
}

void input_circles(component *comp, touch_event *evt){
    if(comp->state == NULL){
        comp->state=calloc(MAX_SLOTS, sizeof(coordinate));
    }
    coordinate *fingers = (coordinate *)comp->state;

    for(int i =0; i < evt->num_touches; i++){
        fingers[i].x = evt->multi_x[i];
        fingers[i].y = evt->multi_y[i];
    }
    for(int i=evt->num_touches; i< MAX_SLOTS; i++){
        fingers[i].x = -1;
        fingers[i].y = -1;
    }
}



/*
void draw_roi_selector(VGfloat x, VGfloat y, VGfloat w, VGfloat h, float off_x, float off_y){
    VGfloat aspect = w/h;
    VGfloat asp_scale = 1.0;
    if(aspect > 1.4) { // limiting factor is height
        asp_scale = h / SENSOR_FULL_HEIGHT;
    }  else {
        asp_scale = w / SENSOR_FULL_WIDTH;
    }

    Fill(0, 0, 255, 1.0);
    Rect(x,y,asp_scale * SENSOR_FULL_WIDTH, asp_scale * SENSOR_FULL_HEIGHT);

    Fill(255, 0, 0, 1.0);
    Rect(x+off_x*asp_scale,y+asp_scale*off_y,
         asp_scale * CROP_WIDTH,asp_scale * CROP_HEIGHT);
}
*/

int collide(rect *bound, int x, int y){
    return bound->x <= x && x <= (bound->x + bound->w) &&
            bound->y <= y && y <= (bound->y + bound->h);
}

void get_local_coord(rect *bound, coordinate *absolute){
    absolute->x -= bound->x;
    absolute->y -= bound->y;
}


void print_touch_event(touch_event *evt){
    fprintf(stderr, "%-10s ",
            evt->event_type == TOUCH_DOWN ? "TOUCH_DOWN" :
            evt->event_type == TOUCH_UP ? "TOUCH_UP" : "TOUCH_DRAG" );
    for(int i = 0; i < max(1,evt->num_touches); i++ ){
        fprintf(stderr,"  POSITION%d: (%03d,%03d)",i,
                evt->multi_x[i],
                evt->multi_y[i]);
    }
    fprintf(stderr, " TOUCHES: %d\n",evt->num_touches);
}

void handle_touch_event(touch_event *evt){
    // handle any touch actions (backwards from draw order)
    size_t num_items = sizeof(components) / sizeof(components[0]);
    static component *last_component = NULL;



    if(last_component != NULL && collide(&(last_component->bounds), evt->x, evt->y)){
        // often the last component gets multiple events
        // so if the current event is still valid, short-circuit the search
        if(last_component->input != NULL){
            last_component->input(last_component, evt);
        }
    }
    else {
        for(int i = num_items; i >= 0; i--){
            if(collide(&(components[i].bounds), evt->x, evt->y))
            {
                if(components[i].input != NULL){
                    components[i].input(&components[i], evt);

                    return; // only one input gets the event
                }
            }
            /*
            if(collide(components[i].bounds, evt->x, evt->y)) {
                components[i].input();
                break; // don't send it to other components
            }*/
        }
    }
}

component* get_component_by_name(char *name, size_t num_components){
    for(int i = 0; i <num_components; i++){
        if(strcmp(components[i].name, name)){
            return &components[i];
        }
    }
    return NULL;
}


void init_ui(ui_state *state, camera_state *cam_state){
    state->num_components = sizeof(components) / sizeof(components[0]);
    //get_component_by_name("roi_selector", state->num_components)->state = &(cam_state->cam_offset);
    components[0].state = &(cam_state->cam_offset);
}

void update_ui(ui_state *state) {
    int i;
//
//    touch_event *evt;
    // get a queue of all touch events
    /*while(lfds711_queue_bss_dequeue( &(state->event_queue), NULL, (void**)&evt)){
        fprintf(stderr, "Dequeued: %-", )
        // handle the event
        fprintf(stderr, "%-10s TRACKING_ID: %03d   POSITION: (%03d,%03d)\n",
            isDown ?  "TOUCH_DOWN" : "TOUCH_UP",
            active_slot->tracking_id, active_slot->position_x, active_slot->position_y);
        // destroy the event
        destroy_event(evt);
    }*/



    // redraw each component
    for(i = 0; i < state->num_components; i++){
        if(components[i].draw != NULL) {
            components[i].draw(&components[i]);
        }
    }


    //int i = components[0].draw;

    // draw stuff
}


void deinit_ui(ui_state *state){
    for(int i = 0; i < state->num_components; i++){
        if(components[i].state != NULL) {
            free(components[i].state);
        }
    }
}