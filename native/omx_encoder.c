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
