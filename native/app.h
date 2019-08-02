//
// Created by idegani on 7/15/2019.
//

#ifndef IDS_APP_H
#define IDS_APP_H

#include "display.h"
#include "tis_v4l2.h"
#include "omx_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    camera_state *cam_state;
    video_enc_state *vid_state;
    display_state *disp_state;
} app_state ;

#ifdef __cplusplus
}
#endif
#endif //IDS_APP_H
