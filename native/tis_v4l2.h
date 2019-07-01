#include <linux/videodev2.h>
#include <sys/mman.h>
#include <libv4l2.h>
#include <sys/time.h>
#include <time.h>
#include "types.h"

#ifndef IDS_TIS_V4L2_H
#define IDS_TIS_V4L2_H
#ifdef __cplusplus
extern "C" {
#endif
struct cam_buffer {
    void *start;
    size_t length;
};


typedef struct camera_state {
    int fd;
    struct cam_buffer *buffers;
    struct v4l2_buffer buf;
    int fmt_width;
    int fmt_height;
    int fourcc;
    int binning;
    int fps;
    int n_buffers;
    coordinate cam_offset;
} camera_state;




/* Imaging Source UVC Camera Parameters
   from TIS uvc-exctensions/usb3.xml    */
#define TIS_V4L2_EXPOSURE_TIME_US          0x0199e201   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_GAIN_ABS                  0x0199e204   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_TRIGGER                   0x0199e208   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_SOFTWARE_TRIGGER          0x0199e209   // V4L2_CTRL_TYPE_BUTTON
#define TIS_V4L2_TRIGGER_DELAY             0x0199e210   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_STROBE_ENABLE             0x0199e211   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_STROBE_POLARITY           0x0199e212   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_STROBE_EXPOSURE           0x0199e213   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_STROBE_DURATION           0x0199e214   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_STROBE_DELAY              0x0199e215   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_GPOUT                     0x0199e216   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_GPIN                      0x0199e217   // V4L2_CTRL_TYPE_BOOLEAN
//#define TIS_V4L2_ROI_OFFSET_X              0x0199e218   // V4L2_CTRL_TYPE_INTEGER
//#define TIS_V4L2_ROI_OFFSET_Y              0x0199e219   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_ROI_OFFSET_X 26862104
#define TIS_V4L2_ROI_OFFSET_Y 26862105


#define TIS_V4L2_ROI_AUTO_CENTER           0x0199e220   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_OVERRIDE_SCANNING_MODE    0x0199e257   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_TRIGGER                   0x0199e208   // V4L2_CTRL_TYPE_BOOLEAN

#define CLEAR(x) memset(&(x), 0, sizeof(x))

int init_camera(char *device, int res_x, int res_y, unsigned int fourcc, int fps, int binning, camera_state *state);
int xioctl(int fh, int request, void *arg);
struct cam_buffer *get_frame(camera_state *state);
int deinit_camera(camera_state *state);
void queue_buffer(camera_state *state);

void get_roi_offset(camera_state *state, coordinate *coord);
void set_roi_offset(camera_state *state, coordinate *coord);
void set_roi_auto(camera_state *state, int roi_auto);

#ifdef __cplusplus
}
#endif
#endif