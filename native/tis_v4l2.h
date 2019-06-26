#include <linux/videodev2.h>
#include <sys/mman.h>
#include <libv4l2.h>

#ifndef TIS_V4L2_H
#define TIS_V4L2_H

struct buffer {
  void *start;
  size_t length;
};

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
#define TIS_V4L2_ROI_OFFSET_X              0x0199e218   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_ROI_OFFSET_Y              0x0199e219   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_ROI_AUTO_CENTER           0x0199e220   // V4L2_CTRL_TYPE_BOOLEAN
#define TIS_V4L2_OVERRIDE_SCANNING_MODE    0x0199e257   // V4L2_CTRL_TYPE_INTEGER
#define TIS_V4L2_TRIGGER                   0x0199e208   // V4L2_CTRL_TYPE_BOOLEAN

#define CLEAR(x) memset(&(x), 0, sizeof(x))

void xioctl(int fh, int request, void *arg);


#endif