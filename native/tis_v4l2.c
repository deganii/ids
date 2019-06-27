#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "tis_v4l2.h"

static char* get_fourcc_str(char *fourcc_str, int fourcc)
{
    if (fourcc_str == NULL)
        return NULL;
    fourcc_str[0] = (char)(fourcc & 0xFF);
    fourcc_str[1] = (char)((fourcc >> 8) & 0xFF);
    fourcc_str[2] = (char)((fourcc >> 16) & 0xFF);
    fourcc_str[3] = (char)((fourcc >> 24) & 0xFF);
    fourcc_str[4] = 0;
    return fourcc_str;
}

int init_video(char *device, int res_x, int res_y, unsigned int fourcc, int fps, int binning, video_state *state) {
    struct v4l2_format              fmt;
    struct v4l2_requestbuffers      req;
    struct v4l2_control             ctrl;
    enum v4l2_buf_type              type;
    int                             fd, n_buffers, i;

    fd = v4l2_open(device, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        fprintf(stderr, "Cannot open device %s\n", device);
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = res_x;
    fmt.fmt.pix.height      = res_y;
    fmt.fmt.pix.pixelformat = fourcc; // v4l2_fourcc('G','R','E','Y');;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    xioctl(fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != fourcc) {
        char fourcc_str[5];
        printf("Libv4l didn't accept %s format. Can't proceed.\n",
                get_fourcc_str((char*)fourcc_str,fourcc));
        exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != res_x) || (fmt.fmt.pix.height != res_y))
        printf("Warning: driver is sending image at %dx%d\n",
               fmt.fmt.pix.width, fmt.fmt.pix.height);

    struct v4l2_streamparm setfps;
    CLEAR(setfps);
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = fps;
    xioctl(fd, VIDIOC_S_PARM, &setfps);
    printf("Framerate is: %d\n", setfps.parm.capture.timeperframe.denominator);

    CLEAR(ctrl);
    ctrl.id = TIS_V4L2_OVERRIDE_SCANNING_MODE;
    ctrl.value = binning;
    xioctl(fd, VIDIOC_S_CTRL, &ctrl);
    printf("Binning is: %d\n", ctrl.value);
    state->binning = ctrl.value;

    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);
    state->buffers = calloc(req.count, sizeof(struct buffer));

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        CLEAR(state->buf);

        state->buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        state->buf.memory      = V4L2_MEMORY_MMAP;
        state->buf.index       = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, &(state->buf));

        state->buffers[n_buffers].length = state->buf.length;
        state->buffers[n_buffers].start = v4l2_mmap(NULL, state->buf.length,
                                             PROT_READ | PROT_WRITE, MAP_SHARED,
                                             fd, state->buf.m.offset);

        if (MAP_FAILED == state->buffers[n_buffers].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < n_buffers; ++i) {
        CLEAR(state->buf);
        state->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        state->buf.memory = V4L2_MEMORY_MMAP;
        state->buf.index = i;
        xioctl(fd, VIDIOC_QBUF, &(state->buf));
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);

    state->fd = fd;
    state->fmt_width = fmt.fmt.pix.width;
    state->fmt_height = fmt.fmt.pix.height;
    state->fps = setfps.parm.capture.timeperframe.denominator;
    state->n_buffers = n_buffers;
    return 1;
}

struct buffer *get_frame(video_state *state){
    static int r;
    static fd_set fds;
    static struct timeval tv;
    do {
        FD_ZERO(&fds);
        FD_SET(state->fd, &fds);

        // Timeout
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(state->fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno = EINTR)));
    if (r == -1) {
        perror("select");
        printf("Error in get_frame() %s\n", strerror(errno));
        exit(EXIT_FAILURE);
        //return errno;
    }
    state->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    state->buf.memory = V4L2_MEMORY_MMAP;
    xioctl(state->fd, VIDIOC_DQBUF, &(state->buf));
    return &(state->buffers[state->buf.index]);
}

void queue_buffer(video_state *state){
    xioctl(state->fd, VIDIOC_QBUF, &(state->buf));
}


int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
            r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
            fprintf(stderr, "Error %d, %s\n", errno, strerror(errno));
            //exit(EXIT_FAILURE);
            return -1;
    }
    return r;
}

void get_formats(int fh)
{
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum frmsize;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
    char current_fourcc[5];

    while (xioctl(fh, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;
        while (xioctl(fh, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
            if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE){
                // append list frmsize.discrete.width, frmsize.discrete.height));
            }else if(frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE){
                // append frmsize.stepwise.step_width, frmsize.stepwise.step_height));
            }
            frmsize.index++;
        }
        get_fourcc_str(current_fourcc, frmsize.pixel_format);
        // append dict (fourcc => list(resolutions)) PyDict_SetItemString(pDict, current_fourcc, pList);
        fmtdesc.index++;
    }
    return;
}


int deinit_video(video_state *state){
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(state->fd, VIDIOC_STREAMOFF, &type);
    for (int i = 0; i < state->n_buffers; ++i)
        v4l2_munmap(state->buffers[i].start, state->buffers[i].length);
    return v4l2_close(state->fd);
}

