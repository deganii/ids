#include <mtdev.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <search.h>
#ifndef TOUCH_H
#define TOUCH_H


/*
typedef struct {

} SlotItem;


typedef struct {
    SlotItem *items;
    int numItems;
    int tracking_id;
    int x;
    int y;
} Slot;

*/
typedef struct
{
    struct mtdev dev;
	int fd;
	//Slot slots[5];
} TouchState;



int init_touch(char *device, TouchState *state);
int deinit_touch(TouchState*state);
void loop_device(TouchState*state);

#endif