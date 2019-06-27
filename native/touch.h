#include <mtdev.h>
#include <mtdev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef IDS_TOUCH_H
#define IDS_TOUCH_H
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

typedef struct
{
    struct mtdev dev;
	int fd;
	int last_touch_slot;
    enum touch_event_type last_touch_event;
	int num_active_touches;
	touch_slot slots[MAX_SLOTS];
} TouchState;

int init_touch(char *device, TouchState *state);
int deinit_touch(TouchState*state);
void loop_device(TouchState*state);

#endif