#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include "display.c"
#include "egl_render.h"

static volatile sig_atomic_t KILLED = 0;
#define CAM_RES_X 800
#define CAM_RES_Y 480
#define FPS_INTERVAL 100

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

void *egl_render_thread(void *arg){
    display_state *ds = (display_state*)arg;
    egl_component_init(ds);
    int rgb_size = CAM_RES_X*CAM_RES_Y*3;
    char buffer[CAM_RES_X*CAM_RES_Y*3];
    struct timespec                 start, finish, diff_t;
    for(int i = 0; !KILLED; i++){
        memset(buffer, i%255, rgb_size);
        egl_render_buffer(buffer, rgb_size);
        // update at about 30FPS
        usleep(30000);
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
                printf("VIDEO THREAD FPS: %.2f, Calculated over %06d frames, Total time: %.2fs\n",
                       (i - FPS_INTERVAL) / diff_secs, i-FPS_INTERVAL, diff_secs);
                fflush(stdout);

            }
        }

    }
    egl_component_deinit(ds);
    return 0;
}

int main(int argc, char **argv)
{
    display_state                   disp_state;
    init_display(&disp_state, CAM_RES_X, CAM_RES_Y, 1);
    signal(SIGINT, sig_handler);
    struct timespec                 start, finish, diff_t;
    // start rendering in another thread
    static pthread_t video_thread;

    pthread_create(&video_thread, NULL, egl_render_thread, &disp_state);

    for (int i = 0; !KILLED; i++) {
        vgSetPixels(0, 0, disp_state.vg_img, 0, 0, CAM_RES_X, CAM_RES_Y);
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
                printf("MAIN  THREAD FPS: %.2f, Calculated over %06d frames, Total time: %.2fs\n",
                       (i - FPS_INTERVAL) / diff_secs, i-FPS_INTERVAL, diff_secs);
                fflush(stdout);

            }
        }

    }

    deinit_display(&disp_state);
    pthread_join(video_thread, NULL);
    return 0;
}