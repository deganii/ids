// cc -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -I/opt/vc/include/ -I/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/lib/ -L/opt/vc/src/hello_pi/libs/ilclient/ -lv4l2 -lEGL -lGLESv2 -lbcm_host -lopenmaxil  main.c -lilclient -L/opt/vc/lib/ -lvcos -lvchiq_arm -lpthread -lrt -lm -L/opt/vc/src/hello_pi/libs/ilclient -o main.out
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <libv4l2.h>

#include "bcm_host.h"
#include "ilclient.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <VG/openvg.h>
#include <VG/vgu.h>

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

struct buffer {
  void *start;
  size_t length;
};

typedef struct
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
} EGL_STATE_T;

struct egl_manager {
    EGLNativeDisplayType xdpy;
    EGLNativeWindowType xwin;
    EGLNativePixmapType xpix;

    EGLDisplay dpy;
    EGLConfig conf;
    EGLContext ctx;

    EGLSurface win;
    EGLSurface pix;
    EGLSurface pbuf;
    EGLImageKHR image;

    EGLBoolean verbose;
    EGLint major, minor;
};

EGL_STATE_T state, *p_state = &state;
static volatile sig_atomic_t KILLED = 0;

static void sig_handler(int _)
{
    (void)_;
    KILLED = 1;
}

void init_egl(EGL_STATE_T *state)
{
    EGLint num_configs;
    EGLBoolean result;

    static const EGLint attribute_list[] =
	{
	    EGL_RED_SIZE, 5,
	    EGL_GREEN_SIZE, 6,
	    EGL_BLUE_SIZE, 5,
	    EGL_ALPHA_SIZE, 0,
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	    EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT,
	    EGL_NONE
	};

    // get an EGL display connection
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // initialize the EGL display connection
    result = eglInitialize(state->display, NULL, NULL);

    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(state->display, attribute_list, &state->config, 1, &num_configs);
    assert(EGL_FALSE != result);

    result = eglBindAPI(EGL_OPENVG_API);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    state->context = eglCreateContext(state->display,
				      state->config, EGL_NO_CONTEXT,
				      NULL);
    assert(state->context!=EGL_NO_CONTEXT);
}

void init_dispmanx(EGL_DISPMANX_WINDOW_T *nativewindow) {
    int32_t success = 0;
    uint32_t screen_width;
    uint32_t screen_height;

    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    bcm_host_init();

    // create an EGL window surface
    success = graphics_get_display_size(0 /* LCD */,
					&screen_width,
					&screen_height);
    assert( success >= 0 );

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screen_width;
    dst_rect.height = screen_height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = screen_width << 16;
    src_rect.height = screen_height << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element =
	vc_dispmanx_element_add(dispman_update, dispman_display,
				0/*layer*/, &dst_rect, 0/*src*/,
				&src_rect, DISPMANX_PROTECTION_NONE,
				0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

    // Build an EGL_DISPMANX_WINDOW_T from the Dispmanx window
    nativewindow->element = dispman_element;
    nativewindow->width = screen_width;
    nativewindow->height = screen_height;
    vc_dispmanx_update_submit_sync(dispman_update);

    printf("Got a Dispmanx window\n");
}

void egl_from_dispmanx(EGL_STATE_T *state,
		       EGL_DISPMANX_WINDOW_T *nativewindow) {
    EGLBoolean result;

    state->surface = eglCreateWindowSurface(state->display,
					    state->config,
					    nativewindow, NULL );
    assert(state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);
}

void egl_deinit(EGL_STATE_T *state)
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

static void print_def(OMX_PARAM_PORTDEFINITIONTYPE def)
{
   printf("Port %u: %s %u/%u %u %u %s,%s,%s %ux%u %ux%u @%u %u\n",
          def.nPortIndex,
          def.eDir == OMX_DirInput ? "in" : "out",
          def.nBufferCountActual,
          def.nBufferCountMin,
          def.nBufferSize,
          def.nBufferAlignment,
          def.bEnabled ? "enabled" : "disabled",
          def.bPopulated ? "populated" : "not pop.",
          def.bBuffersContiguous ? "contig." : "not cont.",
          def.format.video.nFrameWidth,
          def.format.video.nFrameHeight,
          def.format.video.nStride,
          def.format.video.nSliceHeight,
          def.format.video.xFramerate, def.format.video.eColorFormat);
}


void diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}
//const int CAM_RES_X = 640, CAM_RES_Y = 480, CAM_FPS = 30;   // 100% / 100% when encoding
const int CAM_RES_X = 1280, CAM_RES_Y = 720, CAM_FPS = 30;  // 100% / 50% when encoding
//const int CAM_RES_X = 1280, CAM_RES_Y = 960, CAM_FPS = 25; // 100%
//const int CAM_RES_X = 1920, CAM_RES_Y = 1080, CAM_FPS = 15; // 100%

// NOTE VG_IMAGE has a maximum size of 2048x2048 (4194304) pixels
//const int CAM_RES_X = 2560, CAM_RES_Y = 1440, CAM_FPS = 10; // FAIL
//const int CAM_RES_X = 2560, CAM_RES_Y = 1920, CAM_FPS = 5; // FAIL

int main(int argc, char **argv)
{
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    struct v4l2_requestbuffers      req;
    struct v4l2_control             ctrl;
    enum v4l2_buf_type              type;
    fd_set                          fds;
    struct timeval                  tv;
    int                             r, fd = -1;
    unsigned int                    i, n_buffers;
    char                            *dev_name = "/dev/video0";
    struct buffer                   *buffers;
    VGImage                         vg_img;
    EGL_DISPMANX_WINDOW_T           nativewindow;

    OMX_VIDEO_PARAM_PORTFORMATTYPE  format;
    OMX_PARAM_PORTDEFINITIONTYPE    def;
    COMPONENT_T                     *video_encode = NULL;
    COMPONENT_T                     *list[5];
    OMX_BUFFERHEADERTYPE            *omx_buf;
    OMX_BUFFERHEADERTYPE            *out;
    OMX_ERRORTYPE                   omx_r;
    ILCLIENT_T                      *client;

    char                            *video_file = "capture.h264";
    FILE                            *outf;
    int                             rgb_len = CAM_RES_X*CAM_RES_Y*3;
    struct timespec                 start, finish, diff_t;
    //const int                       TOTAL_FRAMES = 200;

    // stderr to stdout
    dup2(1, 2);


    fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
            perror("Cannot open device");
            exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = CAM_RES_X;
    fmt.fmt.pix.height      = CAM_RES_Y;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY; // v4l2_fourcc('G','R','E','Y');;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    xioctl(fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_GREY) {
            printf("Libv4l didn't accept GREY format. Can't proceed.\n");
            exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != CAM_RES_X) || (fmt.fmt.pix.height != CAM_RES_Y))
            printf("Warning: driver is sending image at %dx%d\n",
                    fmt.fmt.pix.width, fmt.fmt.pix.height);

    struct v4l2_streamparm setfps;
    CLEAR(setfps);
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = CAM_FPS;
    xioctl(fd, VIDIOC_S_PARM, &setfps);
    printf("Framerate is: %d\n", setfps.parm.capture.timeperframe.denominator);

    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    //int binning = 0;
    CLEAR(ctrl);
    ctrl.id = TIS_V4L2_OVERRIDE_SCANNING_MODE;
    ctrl.value = 2;
    xioctl(fd, VIDIOC_S_CTRL, &ctrl);
    printf("Binning is: %d\n", ctrl.value);

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

    init_egl(p_state);
    init_dispmanx(&nativewindow);
    egl_from_dispmanx(p_state, &nativewindow);
    if (eglMakeCurrent(state.display, state.surface,
                       state.surface, state.context) == EGL_FALSE) {
        perror("Failed to eglMakeCurrent");
        exit(EXIT_FAILURE);
    }
    vg_img = vgCreateImage(VG_sL_8, CAM_RES_X,CAM_RES_Y, VG_IMAGE_QUALITY_NONANTIALIASED);
    if (vg_img == VG_INVALID_HANDLE) {
        fprintf(stderr, "Can't create vg_img\n");
        fprintf(stderr, "Error code %x\n", vgGetError());
        exit(2);
    }

    int w_offset = ((int)nativewindow.width - (int)fmt.fmt.pix.width) / 2;
    int h_offset = ((int)nativewindow.height - (int)fmt.fmt.pix.height) / 2;
    printf("Native Window Size: (%d,%d)\n", nativewindow.width, nativewindow.height);
    printf("Camera Format Size: (%d,%d)\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    printf("VG_IMAGE Offsets: (%d,%d)\n", w_offset, h_offset);


    // ------------------------------------
    // BEGIN OMX ENCODER INITIALIZATION
    // ------------------------------------


    memset(list, 0, sizeof(list));

    if ((client = ilclient_init()) == NULL) {
      return -3;
    }

    if (OMX_Init() != OMX_ErrorNone) {
      ilclient_destroy(client);
      return -4;
    }

    // create video_encode
    omx_r = ilclient_create_component(client, &video_encode, "video_encode",
                                 ILCLIENT_DISABLE_ALL_PORTS |
                                 ILCLIENT_ENABLE_INPUT_BUFFERS |
                                 ILCLIENT_ENABLE_OUTPUT_BUFFERS);
    if (omx_r != 0) {
      printf
         ("ilclient_create_component() for video_encode failed with %x!\n",
          omx_r);
      exit(1);
    }
    list[0] = video_encode;

    // get current settings of video_encode component from port 200
    memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    def.nVersion.nVersion = OMX_VERSION;
    def.nPortIndex = 200;

    if (OMX_GetParameter
       (ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition,
        &def) != OMX_ErrorNone) {
      printf("%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n",
             __FUNCTION__, __LINE__);
      exit(1);
    }

    print_def(def);

    // Port 200: in 1/1 115200 16 enabled,not pop.,not cont. 320x240 320x240 @1966080 20
    def.format.video.nFrameWidth = CAM_RES_X;
    def.format.video.nFrameHeight = CAM_RES_Y;
    def.format.video.xFramerate = 30 << 16;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth * 3;
    // def.format.video.eColorFormat = OMX_COLOR_FormatL8; // DOESN'T WORK!
    def.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;
    //def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    print_def(def);

    r = OMX_SetParameter(ILC_GET_HANDLE(video_encode),
                        OMX_IndexParamPortDefinition, &def);
    if (r != OMX_ErrorNone) {
      printf
         ("%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n",
          __FUNCTION__, __LINE__, r);
      exit(1);
    }

    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 201;
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    printf("OMX_SetParameter for video_encode:201...\n");
    omx_r = OMX_SetParameter(ILC_GET_HANDLE(video_encode),
                        OMX_IndexParamVideoPortFormat, &format);
    if (omx_r != OMX_ErrorNone) {
      printf
         ("%s:%d: OMX_SetParameter() for video_encode port 201 failed with %x!\n",
          __FUNCTION__, __LINE__, omx_r);
      exit(1);
    }

    OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
    // set current bitrate to 1Mbit
    memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
    bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
    bitrateType.nVersion.nVersion = OMX_VERSION;
    bitrateType.eControlRate = OMX_Video_ControlRateVariable;
    bitrateType.nTargetBitrate = 1000000;
    bitrateType.nPortIndex = 201;
    omx_r = OMX_SetParameter(ILC_GET_HANDLE(video_encode),
                       OMX_IndexParamVideoBitrate, &bitrateType);
    if (omx_r != OMX_ErrorNone) {
      printf
        ("%s:%d: OMX_SetParameter() for bitrate for video_encode port 201 failed with %x!\n",
         __FUNCTION__, __LINE__, omx_r);
      exit(1);
    }


    // get current bitrate
    memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
    bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
    bitrateType.nVersion.nVersion = OMX_VERSION;
    bitrateType.nPortIndex = 201;

    if (OMX_GetParameter
       (ILC_GET_HANDLE(video_encode), OMX_IndexParamVideoBitrate,
       &bitrateType) != OMX_ErrorNone) {
      printf("%s:%d: OMX_GetParameter() for video_encode for bitrate port 201 failed!\n",
            __FUNCTION__, __LINE__);
      exit(1);
    }
    printf("Current Bitrate=%u\n",bitrateType.nTargetBitrate);

    if (ilclient_change_component_state(video_encode, OMX_StateIdle) == -1) {
      printf
         ("%s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed",
          __FUNCTION__, __LINE__);
    }

    if (ilclient_enable_port_buffers(video_encode, 200, NULL, NULL, NULL) != 0) {
      printf("enabling port buffers for 200 failed!\n");
      exit(1);
    }

    if (ilclient_enable_port_buffers(video_encode, 201, NULL, NULL, NULL) != 0) {
      printf("enabling port buffers for 201 failed!\n");
      exit(1);
    }

    ilclient_change_component_state(video_encode, OMX_StateExecuting);

    outf = fopen(video_file, "w");
    if (outf == NULL) {
      printf("Failed to open '%s' for writing video\n", video_file);
      exit(1);
    }

    clock_gettime( CLOCK_MONOTONIC, &start );
    signal(SIGINT, sig_handler);
    for (i = 0; !KILLED; i++) {

            do {
                    FD_ZERO(&fds);
                    FD_SET(fd, &fds);

                    // Timeout
                    tv.tv_sec = 2;
                    tv.tv_usec = 0;

                    r = select(fd + 1, &fds, NULL, NULL, &tv);
            } while ((r == -1 && (errno = EINTR)));
            if (r == -1) {
                    perror("select");
                    return errno;
            }

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            xioctl(fd, VIDIOC_DQBUF, &buf);

            //printf("Got frame %03d, size %d bytes\n", i, buf.bytesused);
            vgImageSubData(vg_img, buffers[buf.index].start, fmt.fmt.pix.width,
                VG_sL_8, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);

            vgSetPixels(w_offset, h_offset, vg_img, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);
            eglSwapBuffers(p_state->display, p_state->surface);

            omx_buf = ilclient_get_input_buffer(video_encode, 200, 1);
            if (omx_buf == NULL) {
                printf("No buffers available\n");
            } else {

             // in the future, try to avoid this RGB conversion and pass the L8 buffer directly
             char *buf_s = buffers[buf.index].start;
             int c2l = 0;
             for(int l2c = 0; l2c < buf.bytesused; l2c++){
                omx_buf->pBuffer[c2l++] = buf_s[l2c];
                omx_buf->pBuffer[c2l++] = buf_s[l2c];
                omx_buf->pBuffer[c2l++] = buf_s[l2c];
             }
             omx_buf->nFilledLen = rgb_len;

             if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_encode), omx_buf) !=
                 OMX_ErrorNone) {
                printf("Error emptying buffer!\n");
             }

             out = ilclient_get_output_buffer(video_encode, 201, 1);

             if (out != NULL) {
                if (out->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                   int q;
                   for (q = 0; q < out->nFilledLen; q++)
                      printf("%x ", out->pBuffer[q]);
                   printf("\n");
                }

                omx_r = fwrite(out->pBuffer, 1, out->nFilledLen, outf);
                if (omx_r != out->nFilledLen) {
                   printf("fwrite: Error emptying buffer: %d!\n", r);
                } else {
                   // printf("Writing frame %d/%d, len %u\n", i, TOTAL_FRAMES, out->nFilledLen);
                }
                out->nFilledLen = 0;
             } else {
                printf("Not getting it :(\n");
             }

             omx_r = OMX_FillThisBuffer(ILC_GET_HANDLE(video_encode), out);
             if (omx_r != OMX_ErrorNone) {
                printf("Error sending buffer for filling: %x\n", omx_r);
             }
            }

            xioctl(fd, VIDIOC_QBUF, &buf);

            if(i%50 == 0) {
                clock_gettime( CLOCK_MONOTONIC, &finish );
                diff(&start, &finish, &diff_t);
                float diff_secs = diff_t.tv_sec + (diff_t.tv_nsec / 1.0e9);
                if(i == 0){
                    printf("\n\n");
                    fflush(stdout);
                }
                printf("\e[0EFPS: %.2f, Calculated over %06d frames, Total time: %.2fs",
                    (i+1) / diff_secs, i, diff_secs);
                fflush(stdout);
            }
    }
    /*clock_gettime( CLOCK_MONOTONIC, &finish );
    diff(&start, &finish, &diff_t);
    float diff_secs = diff_t.tv_sec + (diff_t.tv_nsec / 1.0e9);
    printf("FPS: %.2f, Calculated over %d frames, Total time: %.2fs\n",
        (i+1) / diff_secs, i+1, diff_secs);*/

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < n_buffers; ++i)
            v4l2_munmap(buffers[i].start, buffers[i].length);
    v4l2_close(fd);

    vgDestroyImage(vg_img);
    egl_deinit(&state);

    fclose(outf);

    printf("Teardown.\n");

    printf("disabling port buffers for 200 and 201...\n");
    ilclient_disable_port_buffers(video_encode, 200, NULL, NULL, NULL);
    ilclient_disable_port_buffers(video_encode, 201, NULL, NULL, NULL);

    ilclient_state_transition(list, OMX_StateIdle);
    ilclient_state_transition(list, OMX_StateLoaded);

    ilclient_cleanup_components(list);

    OMX_Deinit();

    ilclient_destroy(client);

    printf("\n\n");
    fflush(stdout);

    return 0;
}