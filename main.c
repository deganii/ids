//  cc -I/opt/vc/include/ -L/opt/vc/lib/ -lv4l2 -lEGL -lGLESv2 -lbcm_host main.c -o main.out

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <libv4l2.h>
#include "bcm_host.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <VG/openvg.h>
#include <VG/vgu.h>


#include <time.h>

/* Imaging Source UVC Camera Parameters */
/* from TIS uvc-exctensions/usb3.xml*/
#define TIS_V4L2_EXPOSURE_TIME_US   0x0199e201
#define TIS_V4L2_GAIN_ABS           0x0199e204
#define TIS_V4L2_ROI_OFFSET_X       0x0199e218
#define TIS_V4L2_ROI_OFFSET_Y       0x0199e219
#define TIS_V4L2_ROI_AUTO_CENTER    0x0199e220


#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
  void *start;
  size_t length;
};

typedef struct tagEsContext
{
    GLint width;
    GLint height;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} EsContext;
EsContext state;

static void get_native_display_size(uint32_t* width, uint32_t* height)
{
    // get display size
    if (graphics_get_display_size(0 /*LCD*/,
                                  width,
                                  height) < 0) {
        perror("Failed to graphics_get_display_size");
        exit(EXIT_FAILURE);
    }
}

static NativeWindowType
create_native_window(uint32_t width, uint32_t height)
{
    VC_RECT_T dst_rect;
    vc_dispmanx_rect_set(&dst_rect, 0, 0, width, height);

    VC_RECT_T src_rect;
    vc_dispmanx_rect_set(&src_rect, 0, 0, (width<<16), (height<<16));

    DISPMANX_DISPLAY_HANDLE_T dispman_display = vc_dispmanx_display_open(0 /*LCD*/);
    DISPMANX_UPDATE_HANDLE_T dispman_update = vc_dispmanx_update_start(0);
    DISPMANX_ELEMENT_HANDLE_T dispman_element =
        vc_dispmanx_element_add(dispman_update,
                                dispman_display,
                                0/*layer*/,
                                &dst_rect,
                                0/*src*/,
                                &src_rect,
                                DISPMANX_PROTECTION_NONE,
                                0/*alpha*/,
                                0/*clamp*/,
                                DISPMANX_NO_ROTATE);
    static EGL_DISPMANX_WINDOW_T nativewindow;
    nativewindow.element = dispman_element;
    nativewindow.width = width;
    nativewindow.height = height;
    vc_dispmanx_update_submit_sync(dispman_update);
    return (NativeWindowType)&nativewindow;
}

void init_ogl(EsContext *state)
{
    // get an EGL display connection
    state->display = eglGetDisplay(state->display);

    // initialize the EGL display connection
    if (eglInitialize(state->display, NULL, NULL) == EGL_FALSE) {
        perror("Failed to eglInitialize");
        exit(EXIT_FAILURE);
    }

    // get an appropriate EGL frame buffer configuration
    static const EGLint attribute_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_BUFFER_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLint num_config = 0;
    EGLConfig config;
    if (eglChooseConfig(state->display, attribute_list,
                        &config, 1, &num_config) == EGL_FALSE) {
        perror("Failed to eglChooseConfig");
        exit(EXIT_FAILURE);
    }

    // bind to OpenGLES API
    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
        perror("Failed to eglBindAPI");
        exit(EXIT_FAILURE);
    }

    // create an EGL rendering context
    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    state->context = eglCreateContext(state->display, config,
                                      EGL_NO_CONTEXT, context_attributes);
    if (state->context == EGL_NO_CONTEXT) {
        perror("Failed to eglCreateContext");
        exit(EXIT_FAILURE);
    }

    // create an EGL Window surface
    get_native_display_size(&state->width, &state->height);
    NativeWindowType nativewindow =
        create_native_window(state->width, state->height);
    state->surface = eglCreateWindowSurface(state->display, config,
                                            nativewindow, NULL);
    if (state->surface == EGL_NO_SURFACE) {
        perror("Failed to eglCreateWindowSurface");
        exit(EXIT_FAILURE);
    }

    // connect the context to the surface
    if (eglMakeCurrent(state->display, state->surface,
                       state->surface, state->context) == EGL_FALSE) {
        perror("Failed to eglMakeCurrent");
        exit(EXIT_FAILURE);
    }
}


void deinit(EsContext *state)
{
	// free the egsImage vgDestroyPaint
	eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	assert(eglGetError() == EGL_SUCCESS);
	eglTerminate(state->display);
	assert(eglGetError() == EGL_SUCCESS);
	eglReleaseThread();
}


static void xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

int main(int argc, char **argv)
{
        struct v4l2_format              fmt;
        struct v4l2_buffer              buf;
        struct v4l2_requestbuffers      req;
        enum v4l2_buf_type              type;
        fd_set                          fds;
        struct timeval                  tv;
        int                             r, fd = -1;
        unsigned int                    i, n_buffers;
        char                            *dev_name = "/dev/video0";
        char                            out_name[256];
        FILE                            *fout;
        struct buffer                   *buffers;
        VGImage                         vg_img;
        char                            fakebuf[640*480];

        memset (fakebuf,128,640*480);

        bcm_host_init();

        // OpenVG needs a surface to draw on, and EGL provides that
        init_ogl(&state);

        //glClearColor(0.0, 0.0, 0.0, 1.0);
        //glClear(GL_COLOR_BUFFER_BIT);
        //glFlush();
        //eglSwapBuffers(state.display, state.surface);
        // also try VG_lL8, VG_sL_8, VG_A_8


            // connect the context to the surface
    if (eglMakeCurrent(state.display, state.surface,
                       state.surface, state.context) == EGL_FALSE) {
        perror("Failed to eglMakeCurrent");
        exit(EXIT_FAILURE);
    }
        vg_img = vgCreateImage(VG_sL_8, 640,480, VG_IMAGE_QUALITY_NONANTIALIASED);

        if (vg_img == VG_INVALID_HANDLE) {
            fprintf(stderr, "Can't create vg_img\n");
            fprintf(stderr, "Error code %x\n", vgGetError());
            exit(2);
        }
        vgClearImage(vg_img, 0, 0, 640, 480);

        fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                perror("Cannot open device");
                exit(EXIT_FAILURE);
        }

        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = 640;
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY; // v4l2_fourcc('G','R','E','Y');;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
        xioctl(fd, VIDIOC_S_FMT, &fmt);
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_GREY) {
                printf("Libv4l didn't accept GREY format. Can't proceed.\n");
                exit(EXIT_FAILURE);
        }
        if ((fmt.fmt.pix.width != 640) || (fmt.fmt.pix.height != 480))
                printf("Warning: driver is sending image at %dx%d\n",
                        fmt.fmt.pix.width, fmt.fmt.pix.height);

        CLEAR(req);
        req.count = 2;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_REQBUFS, &req);

        buffers = calloc(req.count, sizeof(*buffers));
        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                xioctl(fd, VIDIOC_QUERYBUF, &buf);

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start) {
                        perror("mmap");
                        exit(EXIT_FAILURE);
                }
        }

        for (i = 0; i < n_buffers; ++i) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                xioctl(fd, VIDIOC_QBUF, &buf);
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        xioctl(fd, VIDIOC_STREAMON, &type);
        for (i = 0; i < 50; i++) {
                do {
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);
                } while ((r == -1 && (errno = EINTR)));
                if (r == -1) {
                        perror("select");
                        return errno;
                }

                //CLEAR(buf); // do we need to clear?
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                xioctl(fd, VIDIOC_DQBUF, &buf);

                //printf("Got frame %03d, size %d bytes\n", i, buf.bytesused);
                //printf("Value at pixel (%d,%d) = %d\n", 200,300, ((char*)buffers[buf.index].start)[200*640+300]);
                // vgImageSubData(vg_img, fakebuf, fmt.fmt.pix.width,
                vgImageSubData(vg_img, buffers[buf.index].start, fmt.fmt.pix.width,
                    VG_sL_8, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);
                // center the image at (80,0)
                int w_offset = (state.width - fmt.fmt.pix.width) / 2;
                // vgSetPixels(w_offset, 0, vg_img, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);
                vgSetPixels(0, 0, vg_img, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);
                vgDrawImage(vg_img);
                // eglSwapBuffers(state.display, state.surface);
                // do we need this?
                //eglSwapBuffers(state.display, state.surface);


                /*sprintf(out_name, "out%03d.ppm", i);
                fout = fopen(out_name, "w");
                if (!fout) {
                        perror("Cannot open image");
                        exit(EXIT_FAILURE);
                }
                fprintf(fout, "P6\n%d %d 255\n",
                        fmt.fmt.pix.width, fmt.fmt.pix.height);
                fwrite(buffers[buf.index].start, buf.bytesused, 1, fout);
                fclose(fout);*/

                xioctl(fd, VIDIOC_QBUF, &buf);
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);
        for (i = 0; i < n_buffers; ++i)
                v4l2_munmap(buffers[i].start, buffers[i].length);
        v4l2_close(fd);

        vgDestroyImage(vg_img);
        deinit(&state);
        return 0;
}