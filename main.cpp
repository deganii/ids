#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/eventfd.h>
#include <time.h>
#include <string.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <stdbool.h>

// OPENCV Includes
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include <opencv2/opencv.hpp>

//#include <opencv/cv.hpp>

#include <shapes.h>
#include <egl_render.h>


#include "omx_encoder.h"
#include "display.h"
#include "tis_v4l2.h"
#include "touch.h"
#include "ui.h"
#include "imgproc.h"
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif
    //app_state ids_state;
    touch_state tch_state;
    camera_state cam_state;
    video_enc_state vid_state;
    display_state disp_state;
    ui_state iu_state;
    static volatile sig_atomic_t KILLED = 0;
    static pthread_t video_thread;

    static pthread_mutex_t  frame_consumed_lock;
    static pthread_cond_t  frame_consumed;
#ifdef __cplusplus
}
#endif
#define FPS_INTERVAL 100
#define FRAME_READY 0xF1

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void sig_handler(int _)
{
    (void)_;
    KILLED = 1;
}


const void diff(struct timespec *start, struct timespec *stop,
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
template <typename F, typename ... Ts>

double timer(F f, Ts&&...args) {
    clock_t t_begin = std::clock();
    f(std::forward<Ts>(args)...);
    clock_t t_end = std::clock();
    return double(t_end - t_begin) / CLOCKS_PER_SEC;
}


template <typename F>
double timer(F f) {
    clock_t t_begin = std::clock();
    f();
    clock_t t_end = std::clock();
    return double(t_end - t_begin) / CLOCKS_PER_SEC;
}

/* add a fd to fd_set, and update max_fd */
int safe_fd_set(int fd, fd_set* fds, int* max_fd) {
    assert(max_fd != NULL);

    FD_SET(fd, fds);
    if (fd > *max_fd) {
        *max_fd = fd;
    }
    return 0;
}

/* clear fd from fds, update max fd if needed */
int safe_fd_clr(int fd, fd_set* fds, int* max_fd) {
    assert(max_fd != NULL);

    FD_CLR(fd, fds);
    if (fd == *max_fd) {
        (*max_fd)--;
    }
    return 0;
}

void *video_thread_main(void *args){
    struct cam_buffer *frame;
    cv::Mat *opencv_in_gray = NULL;
    cv::Mat *opencv_out_gray = NULL;
    cv::Mat *opencv_rgb = NULL;
    printf("Initialized OpenCV\n");

    int ev_fd = *(int *)args;

    while(!KILLED) {
        frame = get_frame(&cam_state);
        if(opencv_in_gray == NULL) {
            opencv_in_gray = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC1, (unsigned char *) frame->start);
            opencv_out_gray = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC1, (unsigned char *) malloc(frame->length));
            cam_state.processed_frame = opencv_out_gray->data;
        }

        opencv_in_gray->copyTo(*opencv_out_gray);
        //threshold(*opencv_in_gray, *opencv_out_gray, 127, 255,cv::ThresholdTypes::THRESH_BINARY );

        // wake up the main thread because we have a frame ready to display
        eventfd_write(ev_fd, FRAME_READY);

        // now wait until the main thread is done displaying this frame on-screen
        pthread_mutex_lock(&frame_consumed_lock);
        pthread_cond_wait(&frame_consumed, &frame_consumed_lock);
        // queue the buffer back to v4l now that the main thread is done with it
        queue_buffer(&cam_state);
        pthread_mutex_unlock(&frame_consumed_lock);
    }
    delete opencv_in_gray;
    delete opencv_out_gray;
    delete opencv_rgb;
}

int main(int argc, char **argv)
{
    unsigned int                    i;
    char                            *cam_name   = (char *)"/dev/video0";
//    char                            *input_name = (char *)"/dev/input/event0";
//    char                            *video_file = (char *)"capture.h264";
    struct timespec                 start, finish, diff_t;
    double time_save_frame =0.0, time_get_encoder_buffer = 0.0;

    // start the python gui
    char pygui[] = "/usr/bin/python3 /home/pi/ids/main.py";
    FILE *pf;
    pf = popen(pygui, "r");
    if(!pf){
        fprintf(stderr, "Could not launch python \n");
        return 0;
    }

    init_camera(cam_name, CAM_RES_X, CAM_RES_Y, V4L2_PIX_FMT_GREY, CAM_FPS, CAM_BINNING, &cam_state);

    init_display(&disp_state, CAM_RES_X, CAM_RES_Y, 0);

    // note: don't initialize touch/ui since this is currently handled by the python/kivy overlay
//    init_touch(input_name, &tch_state,disp_state.nativewindow.height);
//    init_ui(&iu_state, &cam_state);

    int w_offset = ((int)disp_state.nativewindow.width - (int)cam_state.fmt_width) / 2;
    int h_offset = ((int)disp_state.nativewindow.height - (int)cam_state.fmt_height) / 2;

    printf("Native Window Size: (%d,%d)\n", disp_state.nativewindow.width, disp_state.nativewindow.height);
    printf("Camera Format Size: (%d,%d)\n", cam_state.fmt_width, cam_state.fmt_height);
    printf("Camera ROI Offsets: (%d,%d)\n", cam_state.cam_offset.x, cam_state.cam_offset.y);
    printf("VG_IMAGE Offsets:   (%d,%d)\n", w_offset, h_offset);

    signal(SIGINT, sig_handler);

    Start(disp_state.nativewindow.width, disp_state.nativewindow.height);

    static int r, max_fd;
    static fd_set master;
    FD_ZERO(&master);

    int GUI_SOCKET;
    safe_fd_set(GUI_SOCKET, &master, &max_fd);


    int VIDEO_FRAME_FD = eventfd(0, 0);
    eventfd_t frame_ready;
    if(VIDEO_FRAME_FD == -1){
        handle_error("eventfd");
    }
    safe_fd_set(VIDEO_FRAME_FD, &master, &max_fd);
    pthread_create(&video_thread, NULL, video_thread_main, &VIDEO_FRAME_FD);

    for (i = 0; !KILLED; i++) {
        /* back up master */
        fd_set dup = master;
        do {
            // wait for either a camera frame or a Kivy UI event from our child python process
            r = select(max_fd + 1, &dup, NULL, NULL, NULL);

        } while (!KILLED && (r == -1 && (errno = EINTR)));
        if (!KILLED && r == -1) {
            perror("select");
            printf("Error in main select() %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // check if the python GUI woke us up and handle any UI requests
        if(FD_ISSET(GUI_SOCKET, &dup)){
            // handle any UI requests

        }
        // check if the video thread woke us up and display the latest frame
        if(FD_ISSET(VIDEO_FRAME_FD, &dup)){
            // consume the video thread event
            eventfd_read(VIDEO_FRAME_FD, &frame_ready);

            vgImageSubData(disp_state.vg_img, cam_state.processed_frame, cam_state.fmt_width,
                VG_sL_8, 0, 0, cam_state.fmt_width, cam_state.fmt_height);

            // now tell the video thread we're done with its buffer so it can fetch/process the next frame
            pthread_cond_signal(&frame_consumed);
        }

        // eventually change this to load an identity matrix, transform, etc
        vgSetPixels(w_offset, h_offset, disp_state.vg_img, 0, 0, cam_state.fmt_width, cam_state.fmt_height);
        eglSwapBuffers(disp_state.egl_state.display, disp_state.egl_state.surface);

//        frame = get_frame(&cam_state);
//        if(opencv_in_gray == NULL) {
//            opencv_in_gray = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC1, (unsigned char *) frame->start);
//            opencv_out_gray = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC1, (unsigned char *) malloc(frame->length));
//        }
//
//        opencv_in_gray->data = (unsigned char*)frame->start;
//        opencv_out_gray->data = opencv_in_gray->data;

        //computeThresholdNEON(frame->start,opencv_buffer,frame->length,127,255);


//        threshold(*opencv_in_gray, *opencv_out_gray, 127, 255,cv::ThresholdTypes::THRESH_BINARY );


//            adaptiveBilateralFilter(*opencv_in_gray, *opencv_out_gray, )


//             bilateralFilter(*opencv_in_gray, *opencv_out_gray, 9, 75, 75);

//            adaptiveThreshold(*opencv_in_gray, *opencv_out_gray, 255,
//                              cv::AdaptiveThresholdTypes::ADAPTIVE_THRESH_GAUSSIAN_C,
//                              cv::ThresholdTypes :: THRESH_BINARY,11,2);
//
        // no processing on output grayscale
        //opencv_out_gray->data = opencv_in_gray->data;


//          vgImageSubData(disp_state.vg_img, opencv_out_gray->data, cam_state.fmt_width,
//                           VG_sL_8, 0, 0, cam_state.fmt_width, cam_state.fmt_height);



//          vgImageSubData(disp_state.vg_img, opencv_rgba.data, cam_state.fmt_width,
//                           VG_sRGBA_8888, 0, 0, cam_state.fmt_width, cam_state.fmt_height);


//        vgSetPixels(w_offset, h_offset, disp_state.vg_img, 0, 0, cam_state.fmt_width, cam_state.fmt_height);


//        if( opencv_rgb == NULL){
//            opencv_rgb = new cv::Mat(CAM_RES_X, CAM_RES_Y, CV_8UC3, get_encoder_buffer(&vid_state));
//        }
//        else {
//                time_get_encoder_buffer += timer([&]() {
//                opencv_rgb->data = get_encoder_buffer(&vid_state);
//                });
//        }


        // convert to RGB because OMAX doesn't support luminance videos
//        cv::cvtColor(*opencv_out_gray, *opencv_rgb, cv::COLOR_GRAY2RGB);

//            time_save_frame += timer([&](){
//            save_frame(&vid_state);
//            });

//        queue_buffer(&cam_state);

        // handle touch events
        //process_touch_events(&tch_state, handle_touch_event);
        //update_ui(&iu_state);

//        eglSwapBuffers(disp_state.egl_state.display, disp_state.egl_state.surface);

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

//                printf("Time spent in get_encoder_buffer: %.2fms, Calculated over %06d frames, Total time: %.2fs\n",
//                       1000.0*time_get_encoder_buffer/(i+1), i+1, time_get_encoder_buffer);
//
//
//                printf("Time spent in save_frame: %.2fms, Calculated over %06d frames, Total time: %.2fs\n",
//                      1000.0*time_save_frame/(i+1), i+1, time_save_frame);

                fflush(stdout);

            }
        }
    }

    pthread_join(video_thread, NULL);
    deinit_camera(&cam_state);
    deinit_display(&disp_state);
//    deinit_video(&vid_state);
    //deinit_touch(&tch_state);

    printf("\n\n");
    fflush(stdout);
    fclose(pf);
    return 0;
}