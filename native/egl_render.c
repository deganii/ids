//
// Created by idegani on 7/12/2019.
//
// This file implements a thread that accepts a multi-thread-friendly KHR EGL image and updates it everytime
// a new frame arrives
// https://docs.google.com/file/d/0B_oQNe1U0rhqVnFWdnFFVXFuVlE/edit?usp=sharing
//

#include "egl_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <IL/OMX_Component.h>

#include "bcm_host.h"
#include "ilclient.h"


extern volatile sig_atomic_t KILLED;
static OMX_BUFFERHEADERTYPE* eglBuffer = NULL;
static COMPONENT_T* egl_render = NULL;

void my_fill_buffer_done(void* data, COMPONENT_T* comp)
{

    OMX_STATETYPE state;

    if (OMX_GetState(ILC_GET_HANDLE(egl_render), &state) != OMX_ErrorNone) {
        printf("OMX_FillThisBuffer failed while getting egl_render component state\n");
        return;
    }

    if (state != OMX_StateExecuting)
        return;

    if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone)
        printf("OMX_FillThisBuffer failed in callback\n");
}


int egl_component_init(display_state *disp_state){
    //OMX_ERRORTYPE omx_err;
    OMX_PARAM_PORTDEFINITIONTYPE    def;
//    OMX_IMAGE_PARAM_PORTFORMATTYPE  imagePortFormat;
//    OMX_VIDEO_PARAM_PORTFORMATTYPE  videoFormat;

    OMX_ERRORTYPE                   omx_r;

    if (disp_state == NULL || disp_state->eglKHRImage == NULL)
    {
        printf("display state or eglImage is null.\n");
        exit(1);
    }
    int status = 0;

    if((disp_state->client = ilclient_init()) == NULL)
    {
        return -5;
    }

    if(OMX_Init() != OMX_ErrorNone)
    {
        ilclient_destroy(disp_state->client);
        return -4;
    }

    // callback
    ilclient_set_fill_buffer_done_callback(disp_state->client, my_fill_buffer_done, 0);

    // create egl_render
    if(status == 0 && ilclient_create_component(disp_state->client, &egl_render, "egl_render",
        ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0) {
//          ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS ) != 0) {
        return -14;
    }

    // get current settings of video_encode component from port 220
    memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    def.nVersion.nVersion = OMX_VERSION;
    def.nPortIndex = 220;

    if (OMX_GetParameter
        (ILC_GET_HANDLE(egl_render), OMX_IndexParamPortDefinition,
         &def) != OMX_ErrorNone) {
        printf("%s:%d: OMX_GetParameter() for egl_render port 220 failed!\n",
               __FUNCTION__, __LINE__);
        exit(1);
    }

//    print_image_def(def);


    def.format.video.nFrameWidth = 640;
    def.format.video.nFrameHeight = 480;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth * 4;

//    def.nBufferSize = (unsigned int)(640.0*480.0*1.5);

//    def.nBufferSize = (unsigned int)(640.0*480.0*2.0);
//    def.format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;

    def.nBufferSize = (unsigned int)(640.0*480.0*4);
    def.format.video.eColorFormat = OMX_COLOR_Format32bitABGR8888;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    //def.format.video.xFramerate = 0; // 30 << 16;

    omx_r = OMX_SetParameter(ILC_GET_HANDLE(egl_render),
                             OMX_IndexParamPortDefinition, &def);
    if (omx_r != OMX_ErrorNone) {
        printf("%s:%d: OMX_SetParameter() for egl_render port 220 failed with %x!\n",
                 __FUNCTION__, __LINE__, omx_r);
        exit(1);
    }


    // set the definition of the output EGL image...
    memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    def.nVersion.nVersion = OMX_VERSION;
    def.nPortIndex = 221;

    if (OMX_GetParameter
        (ILC_GET_HANDLE(egl_render), OMX_IndexParamPortDefinition,
         &def) != OMX_ErrorNone) {
        printf("%s:%d: OMX_GetParameter() for egl_render port 221 failed!\n",
               __FUNCTION__, __LINE__);
        exit(1);
    }
    def.format.video.pNativeWindow = disp_state->egl_state.display;

    //    def.format.video.nFrameWidth = 640;
    //    def.format.video.nFrameHeight = 480;
    //    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    //    def.format.video.nStride = def.format.video.nFrameWidth * 4;
    //    def.format.video.nStride = def.format.video.nFrameWidth * 1.5;
    //    def.format.video.nStride = def.format.video.nFrameWidth;
    //
    def.nBufferSize = (unsigned int)(640.0*480.0*1.5);

    //    def.nBufferSize = (unsigned int)(640.0*480.0*2.0);
    //    def.format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;

    //    def.nBufferSize = (unsigned int)(640.0*480.0*4);
    //    def.format.video.eColorFormat = OMX_COLOR_Format32bitABGR8888;
    //    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
        //def.format.video.xFramerate = 0; // 30 << 16;

    omx_r = OMX_SetParameter(ILC_GET_HANDLE(egl_render),
                             OMX_IndexParamPortDefinition, &def);
    if (omx_r != OMX_ErrorNone) {
        printf("%s:%d: OMX_SetParameter(OMX_IndexParamPortDefinition) for egl_render port 221 failed with %x!\n",
               __FUNCTION__, __LINE__, omx_r);
        exit(1);
    }

    //ilclient_wait_for_command_complete(egl_render, OMX_EventPortSettingsChanged, 221);

    // set the format of the output EGL image...
    //    memset(&videoFormat, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    //    videoFormat.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    //    videoFormat.nVersion.nVersion = OMX_VERSION;
    //    videoFormat.nPortIndex = 220;
    //    videoFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
    //    videoFormat.eColorFormat = OMX_COLOR_Format32bitABGR8888;
    //
    //    printf("OMX_SetParameter for video_encode:201...\n");
    //    omx_r = OMX_SetParameter(ILC_GET_HANDLE(egl_render),
    //                             OMX_IndexParamVideoPortFormat, &videoFormat);
    //    if (omx_r != OMX_ErrorNone) {
    //        printf
    //                ("%s:%d: OMX_SetParameter(OMX_IndexParamVideoPortFormat) for egl_render port 221 failed with %x!\n",
    //                 __FUNCTION__, __LINE__, omx_r);
    //        exit(1);
    //    }

    // Set egl_render to idle
    ilclient_change_component_state(egl_render, OMX_StateIdle);

    //setHeader(&imagePortFormat,  sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
    //memset(&imagePortFormat, 0, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
    //memset(&videoFormat, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    //imagePortFormat.nSize = sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE);
    //imagePortFormat.nVersion.nVersion = OMX_VERSION;
    //imagePortFormat.nPortIndex = 220;
    //OMX_GetParameter(ILC_GET_HANDLE(egl_render),
    //                 OMX_IndexParamImagePortFormat, &imagePortFormat);

    if (ilclient_enable_port_buffers(egl_render, 220, NULL, NULL, NULL) != 0) {
        printf("enabling port buffers for 200 failed!\n");
        exit(1);
    }

    //    if (ilclient_enable_port_buffers(egl_render, 221, NULL, NULL, NULL) != 0) {
    //        printf("enabling port buffers for 201 failed!\n");
    //        exit(1);
    //    }


    // Enable the output port and tell egl_render to use the texture as a buffer
    // ilclient_enable_port(egl_render, 220); //THIS BLOCKS SO CAN'T BE USED
    if (OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortEnable, 220, NULL) != OMX_ErrorNone) {
        printf("OMX_CommandPortEnable failed.\n");
        return -3;
    }

    // Enable the output port and tell egl_render to use the texture as a buffer
    //ilclient_enable_port(egl_render, 221); THIS BLOCKS SO CAN'T BE USED
    if (OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone)
    {
        printf("OMX_CommandPortEnable failed.\n");
        return -2;
    }



    if (OMX_UseEGLImage(ILC_GET_HANDLE(egl_render), &eglBuffer, 221, NULL, disp_state->eglKHRImage) != OMX_ErrorNone)
    {
        printf("OMX_UseEGLImage failed.\n");
        return -1;
    }

    // Set egl_render to executing
    ilclient_change_component_state(egl_render, OMX_StateExecuting);

    return 0;
}

void egl_render_buffer(void *rgbBuffer, int len){
//    OMX_BUFFERHEADERTYPE   *out;

    printf("ilclient_get_input_buffer\n");
    eglBuffer = ilclient_get_input_buffer(egl_render, 220, 1);
    if (eglBuffer == NULL) {
        printf("No buffers available\n");
        //return NULL;
    }
    memcpy(eglBuffer->pBuffer, rgbBuffer, len);
    eglBuffer->nFilledLen = len;

    if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(egl_render), eglBuffer) !=
        OMX_ErrorNone) {
        printf("Error emptying cam_buffer!\n");
    }
    printf("ilclient_get_output_buffer\n");
    // this blocks until the output buffer is ready...
    //out = ilclient_get_output_buffer(egl_render, 221, 1);

    printf("egl_render_buffer: OMX_FillThisBuffer\n");
    // Request egl_render to write data to the texture buffer
    if(OMX_FillThisBuffer(ILC_GET_HANDLE(egl_render), eglBuffer) != OMX_ErrorNone)
    {
        printf("OMX_FillThisBuffer failed.\n");
        exit(1);
    }
}

void egl_component_deinit(display_state *disp_state){

    OMX_ERRORTYPE omx_err;
    /*
     * To cleanup egl_render, we need to first disable its output port, then
     * free the output eglBuffer, and finally request the state transition
     * from to Loaded.
     */
    omx_err = OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortDisable, 221, NULL);
    if (omx_err != OMX_ErrorNone)
        printf("Failed OMX_SendCommand\n");

    omx_err = OMX_FreeBuffer(ILC_GET_HANDLE(egl_render), 221, eglBuffer);
    if(omx_err != OMX_ErrorNone)
        printf("OMX_FreeBuffer failed\n");

    OMX_Deinit();
    ilclient_destroy(disp_state->client);
    //return (void *)status;
}
