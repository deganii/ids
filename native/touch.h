#include <mtdev.h>
#include <mtdev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "types.h"

#ifndef IDS_TOUCH_H
#define IDS_TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_SLOTS 5

typedef struct {
    int tracking_id;
    int position_x;
    int position_y;
    int isDown;
} touch_slot;

enum touch_event_type {
    TOUCH_DOWN, TOUCH_UP, TOUCH_DRAG
};

typedef struct touch_event {
    enum touch_event_type event_type;
    int num_touches;
    int x;
    int y;
    // multi-touch
    int multi_x[MAX_SLOTS];
    int multi_y[MAX_SLOTS];
} touch_event;

#define EVENT_QUEUE_SIZE 64

typedef struct
{
    struct mtdev dev;
	int fd;
	int last_touch_slot;
    enum touch_event_type last_touch_event;
	int num_active_touches;
    int display_height;
    queue event_queue;
    q_element  elements[EVENT_QUEUE_SIZE];
	touch_slot slots[MAX_SLOTS];
} touch_state;

int init_touch(char *device, touch_state *state, int display_height);
int deinit_touch(touch_state*state);
void process_touch_events(touch_state *state, void(*callback)(touch_event *));
#ifdef __cplusplus
}
#endif
#endif //IDS_TOUCH_H