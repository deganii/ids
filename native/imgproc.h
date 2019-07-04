//
// Created by idegani on 7/3/2019.
//

#ifndef IDS_IMGPROC_H
#define IDS_IMGPROC_H

#include <stdint.h>
#include <arm_neon.h>

#ifdef __cplusplus
extern "C" {
#endif
void computeThresholdNEON(void *input, void *output, int count, uint8_t threshold, uint8_t highValue);

#ifdef __cplusplus
}
#endif

#endif //IDS_IMGPROC_H
