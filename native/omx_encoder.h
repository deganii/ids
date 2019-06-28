//
// Created by idegani on 6/27/2019.
//
#include "ilclient.h"
#include "tis_v4l2.h"

#ifndef IDS_OMX_ENCODER_H
#define IDS_OMX_ENCODER_H

typedef struct video_enc_state {
    COMPONENT_T            *video_encode;
    OMX_BUFFERHEADERTYPE   *omx_buf;
    ILCLIENT_T             *client;
    FILE                   *outf;
    int                    rgb_len;
    OMX_BUFFERHEADERTYPE   *out;
    COMPONENT_T            *list[5];
} video_enc_state;

void print_def(OMX_PARAM_PORTDEFINITIONTYPE def);

int init_video_encoder(char *video_file, int resx, int resy, video_enc_state *state);
void save_frame(struct buffer *pBuffer, video_enc_state *pState);
void deinit_video(video_enc_state *pState);

#endif //IDS_OMX_ENCODER_H
