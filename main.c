// cc -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -I/opt/vc/include/ -I/opt/vc/src/hello_pi/libs/ilclient -L/opt/vc/lib/ -L/opt/vc/src/hello_pi/libs/ilclient/ -lv4l2 -lEGL -lGLESv2 -lbcm_host -lopenmaxil  main.c -lilclient -L/opt/vc/lib/ -lvcos -lvchiq_arm -lpthread -lrt -lm -L/opt/vc/src/hello_pi/libs/ilclient -o main.out
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <linux/videodev2.h>

#include "ilclient.h"
#include "display.h"
#include "tis_v4l2.h"
#include "touch.h"
#include "ui.h"


EGL_STATE_T state, *p_state = &state;
TouchState touch_state, *p_touch_state = &touch_state;
video_state vid_state, *p_vid_state= &vid_state;

static volatile sig_atomic_t KILLED = 0;

static void sig_handler(int _)
{
    (void)_;
    KILLED = 1;
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
const int CAM_RES_X = 1280, CAM_RES_Y = 720, CAM_FPS = 30, CAM_BINNING=2;  // 100% / 17-19 when encoding
//const int CAM_RES_X = 1280, CAM_RES_Y = 960, CAM_FPS = 25; // 100%
//const int CAM_RES_X = 1920, CAM_RES_Y = 1080, CAM_FPS = 15; // 100%
// NOTE VG_IMAGE has a maximum size of 2048x2048 (4194304) pixels
//const int CAM_RES_X = 2560, CAM_RES_Y = 1440, CAM_FPS = 10; // FAIL
//const int CAM_RES_X = 2560, CAM_RES_Y = 1920, CAM_FPS = 5; // FAIL


int main(int argc, char **argv)
{
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    int                             r;
    unsigned int                    i;
    char                            *dev_name = "/dev/video0";
    char                            *input_name = "/dev/input/event0";
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

    init_touch(input_name, p_touch_state);

    init_video(dev_name,CAM_RES_X,CAM_RES_Y,V4L2_PIX_FMT_GREY,CAM_FPS,CAM_BINNING,p_vid_state);
    fmt.fmt.pix.width = vid_state.fmt_width;
    fmt.fmt.pix.height = vid_state.fmt_height;

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
    struct buffer*frame;
    for (i = 0; !KILLED; i++) {

            frame = get_frame(&vid_state);
            buf = vid_state.buf;

            vgImageSubData(vg_img, frame->start, fmt.fmt.pix.width,
                VG_sL_8, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);
            vgSetPixels(w_offset, h_offset, vg_img, 0, 0, fmt.fmt.pix.width, fmt.fmt.pix.height);

            build_ROI_selector(100,100,200,200, 1140, 650);

            eglSwapBuffers(p_state->display, p_state->surface);

            omx_buf = ilclient_get_input_buffer(video_encode, 200, 1);
            if (omx_buf == NULL) {
                printf("No buffers available\n");
            } else {

             // in the future, try to avoid this RGB conversion and pass the L8 buffer directly
             char *buf_s = frame->start; //buffers[buf.index].start;
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

            queue_buffer(&vid_state);

            // handle touch events
            loop_device(p_touch_state);

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


    deinit_video(p_vid_state);

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