//
// Created by idegani on 6/27/2019.
//
#include <stdio.h>
#include "omx_encoder.h"

void print_def(OMX_PARAM_PORTDEFINITIONTYPE def)
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

int init_video_encoder(char *video_file, int resx, int resy, video_enc_state *state) {

    OMX_VIDEO_PARAM_PORTFORMATTYPE  format;
    OMX_PARAM_PORTDEFINITIONTYPE    def;
    OMX_ERRORTYPE                   omx_r;

    memset(state->list, 0, sizeof(state->list));

    if ((state->client = ilclient_init()) == NULL) {
        return -3;
    }

    if (OMX_Init() != OMX_ErrorNone) {
        ilclient_destroy(state->client);
        return -4;
    }

    // create video_encode
    omx_r = ilclient_create_component(state->client, &(state->video_encode), "video_encode",
                                      ILCLIENT_DISABLE_ALL_PORTS |
                                      ILCLIENT_ENABLE_INPUT_BUFFERS |
                                      ILCLIENT_ENABLE_OUTPUT_BUFFERS);
    if (omx_r != 0) {
        printf
                ("ilclient_create_component() for video_encode failed with %x!\n",
                 omx_r);
        exit(1);
    }
    state->list[0] = state->video_encode;

    // get current settings of video_encode component from port 200
    memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    def.nVersion.nVersion = OMX_VERSION;
    def.nPortIndex = 200;

    if (OMX_GetParameter
                (ILC_GET_HANDLE(state->video_encode), OMX_IndexParamPortDefinition,
                 &def) != OMX_ErrorNone) {
        printf("%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n",
               __FUNCTION__, __LINE__);
        exit(1);
    }

    print_def(def);
//    set_tunnel();
//            ilclient_setup_tunnel()

    // Port 200: in 1/1 115200 16 enabled,not pop.,not cont. 320x240 320x240 @1966080 20
    def.format.video.nFrameWidth = resx;
    def.format.video.nFrameHeight = resy;
    def.format.video.xFramerate = 30 << 16;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth * 3;
    // def.format.video.eColorFormat = OMX_COLOR_FormatL8; // DOESN'T WORK!
    def.format.video.eColorFormat = OMX_COLOR_Format24bitRGB888;
    //def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    print_def(def);

    omx_r = OMX_SetParameter(ILC_GET_HANDLE(state->video_encode),
                             OMX_IndexParamPortDefinition, &def);
    if (omx_r != OMX_ErrorNone) {
        printf
                ("%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n",
                 __FUNCTION__, __LINE__, omx_r);
        exit(1);
    }

    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 201;
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    printf("OMX_SetParameter for video_encode:201...\n");
    omx_r = OMX_SetParameter(ILC_GET_HANDLE(state->video_encode),
                             OMX_IndexParamVideoPortFormat, &format);
    if (omx_r != OMX_ErrorNone) {
        printf
                ("%s:%d: OMX_SetParameter() for video_encode port 201 failed with %x!\n",
                 __FUNCTION__, __LINE__, omx_r);
        exit(1);
    }

    OMX_VIDEO_PARAM_BITRATETYPE bitrateType;

    memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
    bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
    bitrateType.nVersion.nVersion = OMX_VERSION;
    bitrateType.eControlRate = OMX_Video_ControlRateVariable;

    // set current bitrate to 10 Mbits
    // For reference, a class 10 SD card should be able to do 10MB/sec (or 80Mbit)
    bitrateType.nTargetBitrate = 1*1000000;
    bitrateType.nPortIndex = 201;
    omx_r = OMX_SetParameter(ILC_GET_HANDLE(state->video_encode),
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
                (ILC_GET_HANDLE(state->video_encode), OMX_IndexParamVideoBitrate,
                 &bitrateType) != OMX_ErrorNone) {
        printf("%s:%d: OMX_GetParameter() for video_encode for bitrate port 201 failed!\n",
               __FUNCTION__, __LINE__);
        exit(1);
    }
    printf("Current Bitrate=%u\n",bitrateType.nTargetBitrate);

    if (ilclient_change_component_state(state->video_encode, OMX_StateIdle) == -1) {
        printf
                ("%s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed",
                 __FUNCTION__, __LINE__);
    }

    if (ilclient_enable_port_buffers(state->video_encode, 200, NULL, NULL, NULL) != 0) {
        printf("enabling port buffers for 200 failed!\n");
        exit(1);
    }

    if (ilclient_enable_port_buffers(state->video_encode, 201, NULL, NULL, NULL) != 0) {
        printf("enabling port buffers for 201 failed!\n");
        exit(1);
    }

    ilclient_change_component_state(state->video_encode, OMX_StateExecuting);

    state->outf = fopen(video_file, "w");
    if (state->outf == NULL) {
        printf("Failed to open '%s' for writing video\n", video_file);
        exit(1);
    }

    state->rgb_len = resx*resy*3;
    return 0;
}

OMX_U8 *get_encoder_buffer(video_enc_state *pState){
    pState->omx_buf = ilclient_get_input_buffer(pState->video_encode, 200, 1);
    if (pState->omx_buf == NULL) {
        printf("No buffers available\n");
        return NULL;
    }
    return pState->omx_buf->pBuffer;
}

void save_frame(video_enc_state *state) {
    OMX_BUFFERHEADERTYPE   *out;
    OMX_ERRORTYPE          omx_r;

    //state->omx_buf = ilclient_get_input_buffer(state->video_encode, 200, 1);

    if (state->omx_buf == NULL) {
        printf("No buffers available\n");
    } else {

        // assume our buffer is filled
        state->omx_buf->nFilledLen = state->rgb_len;

        if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(state->video_encode), state->omx_buf) !=
            OMX_ErrorNone) {
            printf("Error emptying cam_buffer!\n");
        }

        // this blocks until the output buffer is ready...
        out = ilclient_get_output_buffer(state->video_encode, 201, 1);

        if (out != NULL) {
            if (out->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                int q;
                for (q = 0; q < out->nFilledLen; q++)
                    printf("%x ", out->pBuffer[q]);
                printf("\n");
            }

            omx_r = fwrite(out->pBuffer, 1, out->nFilledLen, state->outf);
            if (omx_r != out->nFilledLen) {
                printf("fwrite: Error emptying cam_buffer: %d!\n", omx_r);
            } else {
                // printf("Writing frame %d/%d, len %u\n", i, TOTAL_FRAMES, out->nFilledLen);
            }
            out->nFilledLen = 0;
        } else {
            printf("Not getting it :(\n");
        }

        omx_r = OMX_FillThisBuffer(ILC_GET_HANDLE(state->video_encode), out);
        if (omx_r != OMX_ErrorNone) {
            printf("Error sending cam_buffer for filling: %x\n", omx_r);
        }
    }

}

void deinit_video(video_enc_state *state) {

    fclose(state->outf);

    printf("Teardown.\n");

    printf("disabling port buffers for 200 and 201...\n");
    ilclient_disable_port_buffers(state->video_encode, 200, NULL, NULL, NULL);
    ilclient_disable_port_buffers(state->video_encode, 201, NULL, NULL, NULL);

    ilclient_state_transition(state->list, OMX_StateIdle);
    ilclient_state_transition(state->list, OMX_StateLoaded);

    ilclient_cleanup_components(state->list);

    OMX_Deinit();

    ilclient_destroy(state->client);
}
