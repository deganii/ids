//
// Created by idegani on 7/1/2019.
//

#include "gpu_fft_pi.h"

// convert a buffer of 8-bit values to an array of GPU_FFT_COMPLEX structs
void convert_buffer(char *img, struct GPU_FFT_COMPLEX *output, int N){
    for(int i = 0; i < N; i++){
        output[i].re = (float)img[i];
    }
}

// 1d FFT - GPU accelerated
void fft(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N){

}

void ifft(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N){

}

// 2d FFT - GPU accelerated - data must be size N x N square
void fft2d(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N){

}

void ifft2d(struct GPU_FFT_COMPLEX *src, struct GPU_FFT_COMPLEX *dest, int N){

}