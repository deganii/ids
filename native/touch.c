#include "touch.h"

/* year-proof millisecond event time */
typedef __u64 mstime_t;

void print_event(const struct input_event *ev) {
	static const mstime_t ms = 1000;
	static int slot;
	int ignoreEvent = 0;
	mstime_t evtime = ev->time.tv_usec / ms + ev->time.tv_sec * ms;
	if (ev->type == EV_ABS && ev->code == ABS_MT_SLOT)
		slot = ev->value;
    char *code_str = 0;
    switch(ev->code) {
        case ABS_MT_SLOT:
			code_str="ABS_MT_SLOT";
			break;
        case ABS_MT_POSITION_X:
			code_str = "ABS_MT_POSITION_X";
			break;
        case ABS_MT_POSITION_Y:
			code_str = "ABS_MT_POSITION_Y";
			break;
        case ABS_MT_TRACKING_ID:
			code_str = "ABS_MT_TRACKING_ID";
			break;
        default:
            ignoreEvent =1;
		/*case BTN_TOUCH:
			code_str = "BTN_TOUCH";
			break;
		case ABS_X:
			code_str = "ABS_X";
			break;
		case ABS_Y:
			code_str = "ABS_Y";
			break;*/
    }
    /*
    if(!code_str){
        char unknown[10];// = char[10];
        code_str = (char *)unknown;
        sprintf(code_str, "%04x", ev->code );
    }*/
    if(!ignoreEvent) {
        fprintf(stderr, "TIME: %012llx SLOT: %02d TYPE: %01d CODE: %-20s VALUE: %d\n",
                evtime, slot, ev->type, code_str, ev->value);
    }
}

void print_state(touch_state *state) {
    // print the current disp_egl_state
    int active_slot_id = (int)(state->last_touch_slot);

    if(active_slot_id > -1){
        touch_slot active_slot = state->slots[active_slot_id];
        if(active_slot.tracking_id > -1){
            fprintf(stderr, "ACTIVE SLOT: %02d TRACKING_ID: %03d  POSITION: (%03d,%03d)\n",
                active_slot_id, active_slot.tracking_id, active_slot.position_x, active_slot.position_y);
        } else{
            fprintf(stderr, "NO ACTIVE SLOTS\n");
        }
        for(int i = 0; i < MAX_SLOTS; i++){
            if(i != active_slot_id && state->slots[i].tracking_id > -1){
                fprintf(stderr, "OTHER  SLOT: %02d TRACKING_ID: %03d  POSITION: (%03d,%03d)\n",
                    i, active_slot.tracking_id, active_slot.position_x, active_slot.position_y);
            }
        }
	}
}

#define CHECK(dev, name)			\
	if (mtdev_has_mt_event(dev, name))	\
		fprintf(stderr, "   %s\n", #name)

static void show_props(const struct mtdev *dev)
{
	fprintf(stderr, "supported mt events:\n");
	CHECK(dev, ABS_MT_SLOT);
	CHECK(dev, ABS_MT_POSITION_X);
	CHECK(dev, ABS_MT_POSITION_Y);
	CHECK(dev, ABS_MT_BLOB_ID);
	CHECK(dev, ABS_MT_TRACKING_ID);
	CHECK(dev, ABS_MT_PRESSURE);
}

int deinit_touch(touch_state*state)
{
    mtdev_close(&(state->dev)); // close device
    ioctl(state->fd, EVIOCGRAB, 0); // ungrab device
    return close(state->fd); // close I/O
}


static void report_drag(touch_state *state, touch_slot *active_slot){
    fprintf(stderr, "TOUCH_DRAG TRACKING_ID: %03d  ", active_slot->tracking_id);
    if(state->num_active_touches>1){
        for(int i = 0; i < MAX_SLOTS; i++ ){
            if(state->slots[i].isDown){
                fprintf(stderr,"  POSITION%d: (%03d,%03d)",i,
                        state->slots[i].position_x,
                        state->slots[i].position_y);
            }
        }
    } else {
        fprintf(stderr,"  POSITION: (%03d,%03d)",
                active_slot->position_x,
                active_slot->position_y);
    }
    fprintf(stderr, "\n");

}

static void report_touch(touch_slot *active_slot, int isDown){
    fprintf(stderr, "%-10s TRACKING_ID: %03d   POSITION: (%03d,%03d)\n",
            isDown ?  "TOUCH_DOWN" : "TOUCH_UP",
            active_slot->tracking_id, active_slot->position_x, active_slot->position_y);
}


static int process_event(touch_state*state, const struct input_event *ev)
{
    int active_slot_id = (int)(state->last_touch_slot);
    touch_slot *active_slot = &(state->slots[active_slot_id]);

    switch(ev->code) {
        case ABS_MT_SLOT:
			state->last_touch_slot = ev->value;
			break;
        case ABS_MT_POSITION_X:
            active_slot->position_x = ev->value;
            if(active_slot->position_x > -1 && active_slot->position_y > -1) {
                if(!active_slot->isDown){
                    active_slot->isDown = 1;
                    state -> num_active_touches++;
                    state -> last_touch_event = TOUCH_DOWN;
                    report_touch(active_slot, 1);
                }
                else {
                    state->last_touch_event = TOUCH_DRAG;
                    report_drag(state, active_slot);
                }
            }
			break;
        case ABS_MT_POSITION_Y:
            active_slot->position_y = ev->value;
            if(active_slot->position_x > -1 && active_slot->position_y > -1) {
                if(!active_slot->isDown){
                    active_slot->isDown = 1;
                    state -> num_active_touches++;
                    state -> last_touch_event = TOUCH_DOWN;
                    report_touch(active_slot, 1);
                }
                else {
                    state->last_touch_event = TOUCH_DRAG;
                    report_drag(state, active_slot);
                }
            }
			break;
        case ABS_MT_TRACKING_ID:
            if(ev->value < 0){
                active_slot->isDown = 0;
                state -> num_active_touches--;
                state -> last_touch_event = TOUCH_UP;
                report_touch(active_slot, 0);
            }
            active_slot->position_x = active_slot->position_y = -1;
            active_slot->tracking_id = ev->value;
			break;
        default:
            break;
    }
    return 1;
}

void loop_device(touch_state*state)
{
    struct input_event ev;
    /* extract all available processed events */
    while (mtdev_get(&(state->dev), state->fd, &ev, 1) > 0) {
        process_event(state, &ev);
    }
}

int init_touch(char *device, touch_state*state)
{
	int fd = open(device, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "error: could not open device: %s\n", device);
		return -1;
	}
	if (ioctl(fd, EVIOCGRAB, 1)) {
		fprintf(stderr, "error: could not grab the device %s\n", device);
		return -1;
	}

	struct mtdev dev;
	int ret = mtdev_open(&(state->dev), fd);
	if (ret) {
		fprintf(stderr, "error: could not open device: %s, %d\n", device, ret);
		return -1;
	}
	show_props(&dev);
    state->fd = fd;
    state->last_touch_slot=0;
    for(int i = 0; i< MAX_SLOTS; i++){
        state->slots[i].tracking_id = -1;
        state->slots[i].position_y = -1;
        state->slots[i].position_x = -1;
    }
	return fd;
}
