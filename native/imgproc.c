//
// Created by idegani on 7/3/2019.
//

#include "imgproc.h"

// fast threshold using SIMD/NEON optimizations for ARMv7
void computeThresholdNEON(void *input, void *output, int count, uint8_t threshold, uint8_t highValue) {
    uint8x16_t thresholdVector = vdupq_n_u8(threshold);
    uint8x16_t highValueVector = vdupq_n_u8(highValue);
    uint8x16_t *__restrict inputVector = (uint8x16_t *)input;
    uint8x16_t *__restrict outputVector = (uint8x16_t *)output;
    for ( ; count > 0; count -= 16, ++inputVector, ++outputVector) {
        *outputVector = (*inputVector > thresholdVector) & highValueVector;
    }
}

// fast threshold using SIMD/NEON optimizations for ARMv7
void computeThreshold2(void *input, void *output, int count, uint8_t threshold, uint8_t highValue) {
    uint8x16_t thresholdVector = vdupq_n_u8(threshold);
    uint8x16_t highValueVector = vdupq_n_u8(highValue);
    uint8x16_t *__restrict inputVector = (uint8x16_t *)input;
    uint8x16_t *__restrict outputVector = (uint8x16_t *)output;
    for ( ; count > 0; count -= 16, ++inputVector, ++outputVector) {
        *outputVector = (*inputVector > thresholdVector) & highValueVector;
    }
}


