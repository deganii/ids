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

// OPENCV Includes
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include <opencv2/opencv.hpp>

//#include <opencv/cv.hpp>

#include <shapes.h>


#include "omx_encoder.h"
#include "display.h"
#include "tis_v4l2.h"
#include "touch.h"
#include "ui.h"
#include "imgproc.h"

#ifdef __cplusplus
extern "C" {
#endif
    touch_state tch_state;
    camera_state cam_state;
    video_enc_state vid_state;
    display_state disp_state;
    ui_state iu_state;
#ifdef __cplusplus
}
#endif
#define FPS_INTERVAL 25

static volatile sig_atomic_t KILLED = 0;
static void sig_handler(int _)
{
    (void)_;
    KILLED = 1;
}

void diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

//const int CAM_RES_X = 640, CAM_RES_Y = 480, CAM_FPS = 30;   // 100% / 100% when encoding
const int CAM_RES_X = 1280, CAM_RES_Y = 720, CAM_FPS = 30, CAM_BINNING=2;  // 100% / 17-19 when encoding
//const int CAM_RES_X = 1280, CAM_RES_Y = 960, CAM_FPS = 25; // 100%
//const int CAM_RES_X = 1920, CAM_RES_Y = 1080, CAM_FPS = 15; // 100%
// NOTE VG_IMAGE has a maximum size of 2048x2048 (4194304) pixels
//const int CAM_RES_X = 2560, CAM_RES_Y = 1440, CAM_FPS = 10; // FAIL
//const int CAM_RES_X = 2560, CAM_RES_Y = 1920, CAM_FPS = 5; // FAIL

using namespace cv;
//using namespace cv::AdaptiveThresholdTypes;


int main(int argc, char **argv)
{
    unsigned int                    i;
    char                            *cam_name   = (char *)"/dev/video0";
    char                            *input_name = (char *)"/dev/input/event0";
    char                            *video_file = (char *)"capture.h264";
    VGImage                         vg_img;
    struct timespec                 start, finish, diff_t;
    coordinate offset;

    init_camera(cam_name, CAM_RES_X, CAM_RES_Y, V4L2_PIX_FMT_GREY, CAM_FPS, CAM_BINNING, &cam_state);

    get_roi_offset(&cam_state, &offset);
    fprintf(stderr, "Offsets: POSITION: (%03d,%03d)\n", offset.x, offset.y);


    init_display(&disp_state);

    //init_touch(input_name, &tch_state,disp_state.nativewindow.height);
    init_video_encoder(video_file, CAM_RES_X, CAM_RES_Y, &vid_state);
    init_ui(&iu_state, &cam_state);


    vg_img = vgCreateImage(VG_sL_8, CAM_RES_X,CAM_RES_Y, VG_IMAGE_QUALITY_NONANTIALIASED);
    if (vg_img == VG_INVALID_HANDLE) {
        fprintf(stderr, "Can't create vg_img\n");
        fprintf(stderr, "Error code %x\n", vgGetError());
        exit(2);
    }


    printf("Camera Offsets: (%d,%d)\n", cam_state.cam_offset.x, cam_state.cam_offset.y);

    int w_offset = ((int)disp_state.nativewindow.width - (int)cam_state.fmt_width) / 2;
    int h_offset = ((int)disp_state.nativewindow.height - (int)cam_state.fmt_height) / 2;
    printf("Native Window Size: (%d,%d)\n", disp_state.nativewindow.width, disp_state.nativewindow.height);
    printf("Camera Format Size: (%d,%d)\n", cam_state.fmt_width, cam_state.fmt_height);
    printf("VG_IMAGE Offsets: (%d,%d)\n", w_offset, h_offset);

    signal(SIGINT, sig_handler);
    struct cam_buffer*frame;

    cv::Mat *opencv_in_gray = NULL;
    cv::Mat *opencv_out_gray = NULL;
    cv::Mat *opencv_rgb = NULL;
    printf("Initialized OpenCV\n");

    Start(disp_state.nativewindow.width, disp_state.nativewindow.height);

    for (i = 0; !KILLED; i++) {

            frame = get_frame(&cam_state);
            if(opencv_in_gray == NULL) {
                opencv_in_gray = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC1, (unsigned char *) frame->start);
                opencv_out_gray = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC1, (unsigned char *) malloc(frame->length));
            }

            opencv_in_gray->data = (unsigned char*)frame->start;
            opencv_out_gray->data = opencv_in_gray->data;

            //computeThresholdNEON(frame->start,opencv_buffer,frame->length,127,255);

            // in-place threshold
            threshold(*opencv_in_gray, *opencv_in_gray, 127, 255,cv::ThresholdTypes::THRESH_BINARY );


//            adaptiveBilateralFilter(*opencv_in_gray, *opencv_out_gray, )


       //      bilateralFilter(*opencv_in_gray, *opencv_out_gray, 9, 75, 75);

//            adaptiveThreshold(*opencv_in_gray, *opencv_out_gray, 255,
//                              cv::AdaptiveThresholdTypes::ADAPTIVE_THRESH_GAUSSIAN_C,
//                              cv::ThresholdTypes :: THRESH_BINARY,11,2);
//
            // no processing on output grayscale
            //opencv_out_gray->data = opencv_in_gray->data;

            vgImageSubData(vg_img, opencv_out_gray->data, cam_state.fmt_width,
//            vgImageSubData(vg_img, frame->start, cam_state.fmt_width,
                VG_sL_8, 0, 0, cam_state.fmt_width, cam_state.fmt_height);
            vgSetPixels(w_offset, h_offset, vg_img, 0, 0, cam_state.fmt_width, cam_state.fmt_height);


            if( opencv_rgb == NULL){
                opencv_rgb = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC3, get_encoder_buffer(&vid_state));
            }
            else {
                opencv_rgb->data =  get_encoder_buffer(&vid_state);
            }


            // convert to RGB because OMAX doesn't support luminance videos
            cv::cvtColor(*opencv_out_gray, *opencv_rgb, cv::COLOR_GRAY2RGB);
            save_frame(&vid_state);

            queue_buffer(&cam_state);

            // handle touch events
            //process_touch_events(&tch_state, handle_touch_event);
            update_ui(&iu_state);

            eglSwapBuffers(disp_state.egl_state.display, disp_state.egl_state.surface);

            if(i%FPS_INTERVAL == 0 && i > 0) {
                if(i == FPS_INTERVAL){
                    clock_gettime( CLOCK_MONOTONIC, &start );
                } else {
                    clock_gettime(CLOCK_MONOTONIC, &finish);
                    diff(&start, &finish, &diff_t);
                    float diff_secs = diff_t.tv_sec + (diff_t.tv_nsec / 1.0e9);
                    if (i == 0) {
                        printf("\n\n");
                        fflush(stdout);
                    }
                    printf("\e[0EFPS: %.2f, Calculated over %06d frames, Total time: %.2fs\n",
                           (i - FPS_INTERVAL) / diff_secs, i-FPS_INTERVAL, diff_secs);
                    fflush(stdout);
                }
            }
    }

    deinit_camera(&cam_state);
    vgDestroyImage(vg_img);
    deinit_display(&disp_state);
    deinit_video(&vid_state);
    //deinit_touch(&tch_state);


    delete opencv_in_gray;
    delete opencv_out_gray;
    delete opencv_rgb;

    printf("\n\n");
    fflush(stdout);

    return 0;
}