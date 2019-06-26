#include "touch.h"

/* year-proof millisecond event time */
typedef __u64 mstime_t;

static int use_event(const struct input_event *ev)
{
#if 0
	return ev->type == EV_ABS && mtdev_is_absmt(ev->code);
#else
	return 1;
#endif
}

void print_event(const struct input_event *ev) {
	static const mstime_t ms = 1000;
	static int slot;
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
		case BTN_TOUCH:
			code_str = "BTN_TOUCH";
			break;
		case ABS_X:
			code_str = "ABS_X";
			break;
		case ABS_Y:
			code_str = "ABS_Y";
			break;
    }
    if(!code_str){
        char unknown[10];// = char[10];
        code_str = (char *)unknown;
        sprintf(code_str, "%04x", ev->code );
    }
    fprintf(stderr, "TIME: %012llx SLOT: %02d TYPE: %01d CODE: %-20s VALUE: %d\n",
		evtime, slot, ev->type, code_str, ev->value);
}

//#define BITMASK(x) (1U << (x))
//#define BITONES(x) (BITMASK(x) - 1U)
//#define GETBIT(m, x) (((m) >> (x)) & 1U)
//#define SETBIT(m, x) (m |= BITMASK(x))
//#define CLEARBIT(m, x) (m &= ~BITMASK(x))
//#define MODBIT(m, x, b) ((b) ? SETBIT(m, x) : CLEARBIT(m, x))

void print_state(const struct mtdev_state *dev_state) {
    // print the current state
    dev_state->


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

int deinit_touch(TouchState*state)
{
    mtdev_close(&(state->dev)); // close device
    ioctl(state->fd, EVIOCGRAB, 0); // ungrab device
    return close(state->fd); // close I/O
}

void loop_device(TouchState*state)
{
    struct input_event ev;
    /* extract all available processed events */
    while (mtdev_get(&(state->dev), state->fd, &ev, 1) > 0) {
        if (use_event(&ev))
            print_event(&ev);
    }
}

int init_touch(char *device, TouchState*state)
{
    hcreate(30);

	int fd = open(device, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "error: could not open device\n");
		return -1;
	}
	if (ioctl(fd, EVIOCGRAB, 1)) {
		fprintf(stderr, "error: could not grab the device\n");
		return -1;
	}

	struct mtdev dev;
	int ret = mtdev_open(&(state->dev), fd);
	if (ret) {
		fprintf(stderr, "error: could not open device: %d\n", ret);
		return -1;
	}
	show_props(&dev);
    state->fd = fd;
	return fd;
}