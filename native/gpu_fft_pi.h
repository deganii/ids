//
// Created by idegani on 7/1/2019.
// Convenience class to implement 1d and 2d FFT's on Rasberry PI GPU
//

#include <gpu_fft.h>

#ifndef IDS_GPU_FFT_H
#define IDS_GPU_FFT_H
#ifdef __cplusplus
extern "C" {
#endif


    // convert a buffer of 8-bit values to an array of GPU_FFT_COMPLEX structs
    void convert_buffer(char *img, struct GPU_FFT_COMPLEX *output, int N);

    // 1d FFT - GPU accelerated
    void fft(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N);
    void ifft(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N);

     // 2d FFT - GPU accelerated - data must be size N x N square
    void fft2d(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N);
    void ifft2d(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N);

#ifdef __cplusplus
}
#endif

#endif //IDS_GPU_FFT_H
