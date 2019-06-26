#include <mtdev.h>
#include <mtdev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef TOUCH_H
#define TOUCH_H
#define MAX_SLOTS 5

typedef struct {
    int tracking_id;
    int position_x;
    int position_y;
} touch_slot;


typedef struct
{
    struct mtdev dev;
	int fd;
	int last_slot;
	touch_slot slots[MAX_SLOTS];
} TouchState;

int init_touch(char *device, TouchState *state);
int deinit_touch(TouchState*state);
void loop_device(TouchState*state);

#endif