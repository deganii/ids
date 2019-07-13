//
// Created by idegani on 7/3/2019.
//
// These performance tests compare image processing using:
// - OpenCV : compiled with V4CC to have OpenCL extensions
// - Straight-C (i.e. for loops)
// - ARM NEON optimizations
// - Raspberry GPU processing
//
// Common functionality such as
// - Thresholding,
// - 2d spatial FFT,
// - Grayscale to RGB conversion
// is tested

// Results on  a Raspberry Pi 3 (Rev. a02082) for a 1024x1024 Image
// Linux raspberrypi 4.19.50-v7+ #1234 SMP Thu Jun 13 11:06:37 BST 2019 armv7l GNU/Linux
//  Naive        Threshold         19.32ms Calculated over 000100 iterations | Total time:   1.93s
//  ARM/NEON     Threshold          1.90ms Calculated over 000100 iterations | Total time:   0.19s
//  OpenCV       Threshold          1.05ms Calculated over 000100 iterations | Total time:   0.10s
//  OpenCV-LUT   Threshold          1.49ms Calculated over 000100 iterations | Total time:   0.15s
//  Naive        RGB_Conversion    41.88ms Calculated over 000100 iterations | Total time:   4.19s
//  OpenCV       RGB_Conversion     2.15ms Calculated over 000100 iterations | Total time:   0.21s
//  Naive        FloatConvert      23.54ms Calculated over 000100 iterations | Total time:   2.35s
//  OpenCV       FloatConvert       2.59ms Calculated over 000100 iterations | Total time:   0.26s
//  Naive        ComplexConvert    25.83ms Calculated over 000100 iterations | Total time:   2.58s
//  OpenCV       ComplexConvert     7.97ms Calculated over 000100 iterations | Total time:   0.80s
//  OpenCV       DFT2d            131.70ms Calculated over 000100 iterations | Total time:  13.17s
//  FFTW/F       DFT2d             24.21ms Calculated over 000100 iterations | Total time:   2.42s
//  GPU FFT      DFT2d            133.60ms Calculated over 000100 iterations | Total time:  13.36s

#include <iostream>
#include <chrono>
#include <vector>

#include <fftw3.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/opencv.hpp>

#include "imgproc.h"
#include <time.h>
#include <unistd.h>
#include <mailbox.h>
#include <gpu_fft_trans.h>


#define TEST_IMG_WIDTH  1024
// #define TEST_IMG_HEIGHT 768
#define TEST_IMG_HEIGHT 1024
#define TEST_IMG_DEPTH  1
#define TEST_RGB_DEPTH  3
#define TEST_ITERATIONS 100

using namespace std::chrono;

char *getTestImage(int width, int height, int bytes_per_pixel){
    // native approach
    char *img =  (char *)malloc(width*height*bytes_per_pixel);
    for(int i = 0; i < width*height; i++ ){
        img[i] = (char)i;
    }
    return img;
}

void grayToRGBTestOpenCV(cv::Mat* img, cv::Mat* output){
    // OpenCV threshold
    cv::cvtColor((cv::Mat)(*(cv::Mat*)img), (cv::Mat)(*(cv::Mat*)output), cv::COLOR_GRAY2RGB);
}

void grayToRGBTestNEON(char *img, char *output, int len){
    // there's no gray to RGB ARMV7 Optimized threshold within Ne10
    // computeThresholdNEON(img,output,len,127,255);
}

void grayToRGBTestNaive(char *img, char *output, int len){
    // naive Gray to RGB Conversion
    int c2l = 0;
    for(int l2c = 0; l2c < len; l2c++){
        output[c2l++] = img[l2c];
        output[c2l++] = img[l2c];
        output[c2l++] = img[l2c];
    }
}

void grayToComplexFloatTestNaive(char *img, GPU_FFT_COMPLEX *output, int len){
    // naive complex float conversion
    for(int r2c = 0; r2c < len; r2c++){
        output[r2c].re = (float)img[r2c];
    }
}

void grayToComplexFloatTestOpenCV(cv::Mat *input, cv::Mat *output, cv::Mat* floats, cv::Mat *zeros){
    cv::Mat planes[] = {*floats, *zeros};
    // opencv complex float conversion
    input -> convertTo(*floats, CV_32F);
    cv::merge(planes, 2, *output);
}

void grayToFloatTestNaive(char *img, float *output, int len){
    // naive complex float conversion
    for(int r2c = 0; r2c < len; r2c++){
        output[r2c] = (float)img[r2c];
    }
}

void grayToFloatTestOpenCV(cv::Mat *input, cv::Mat *output){
    input -> convertTo(*output, CV_32F);
}

void grayToComplexDFTOpenCV(cv::Mat *input,  cv::Mat *floats, cv::Mat *output){
    // convert to float and DFT
    input -> convertTo(*floats, CV_32F);
    cv::dft(*floats, *output);
}

void grayToComplexFFTW(fftwf_plan *plan, cv::Mat *input,  cv::Mat *floats){
    // convert to float and DFT
    input -> convertTo(*floats, CV_32F);
    fftwf_execute(*plan);

    // fftwf_plan_dft_r2c_2d(TEST_IMG_WIDTH, TEST_IMG_HEIGHT, (float*)floats->data, fftwComplex,0);
}

void complexToComplexGPUFFT(cv::Mat *input,  cv::Mat *floats,   cv::Mat *zeros, cv::Mat *output, struct GPU_FFT_TRANS *trans, struct GPU_FFT *fft_pass[2]){

    // this conversion is here to make this is a fair test
    // GPU FFT does not accept real-valued input ...
    cv::Mat planes[] = {*floats, *zeros};
    input -> convertTo(*floats, CV_32F);
    cv::merge(planes, 2, *output);

    gpu_fft_execute(fft_pass[0]);
    gpu_fft_trans_execute(trans);
    gpu_fft_execute(fft_pass[1]);
}



void thresholdTestOpenCV(cv::Mat *img, cv::Mat *output){
    // OpenCV threshold
    cv::threshold(*img, *output, 127, 255, cv::ThresholdTypes::THRESH_BINARY );
}

void lutTestOpenCV(cv::Mat *img, cv::Mat *output, cv::Mat *lut){
    cv::LUT(*img, *lut, *output);
}


void thresholdTestNEON(char *img, char *output, int len){
    // ARMV7 Optimized threshold
    computeThresholdNEON(img,output,len,127,255);
}

void thresholdTestNaive(char *img, char *output, int len){
    // naive threshold
    for(int i = 0; i < len; i++){
        output[i] = (img[i] > 127) ? 255:0;
    }
}


auto run_timed = [](auto&& func, const char *method, const char *operation, auto&&... params) {
            // get time before function invocation
            const auto& start = steady_clock::now();
            // function invocation using perfect forwarding
            for(auto i = 0; i < TEST_ITERATIONS; ++i) {
                std::forward<decltype(func)>(func)(std::forward<decltype(params)>(params)...);
            }
            // get time after function invocation
            const auto& stop = steady_clock::now();
            auto execution_time_msec = duration_cast<milliseconds>(stop - start).count();
            printf("%-12s %-16s %6.2fms Calculated over %06d iterations | Total time: %6.2fs\n",
                   method, operation, (float)execution_time_msec / TEST_ITERATIONS,
                   TEST_ITERATIONS,  (float)execution_time_msec/1000.0);
            return (stop - start) / TEST_ITERATIONS;
};




int main(){

    // ----------------------------------
    // Initialize test image and allocate memory for temp images
    // ----------------------------------
    int len = TEST_IMG_WIDTH*TEST_IMG_HEIGHT;
    char *img = getTestImage(TEST_IMG_WIDTH, TEST_IMG_HEIGHT, TEST_IMG_DEPTH);
    char *output = (char *)malloc(len);
    char *outputRGB = (char *)malloc(len*TEST_RGB_DEPTH);
    char *lut = (char *)calloc(256,sizeof(char));
    float *outputFloat = (float *)malloc(len*sizeof(float));
    float *outputZeros = (float *)malloc(len*sizeof(float));
    float *fftwFloat = (float *)fftwf_malloc(len*sizeof(float));
    fftwf_complex *fftwComplex = (fftwf_complex *)fftwf_malloc(len*sizeof(fftw_complex));
    GPU_FFT_COMPLEX *outputComplex = (GPU_FFT_COMPLEX *)malloc(len*sizeof(GPU_FFT_COMPLEX));
    char wisdom[255];

    int thresh =127;
    for(int i = thresh; i < 256; i++){
        lut[i] = 255;
    }

    cv::Mat *opencv_input = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_8UC1,img);
    cv::Mat *opencv_output = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_8UC1,output);
    cv::Mat *opencv_rgboutput = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_8UC3,outputRGB);
    cv::Mat *opencv_zeros = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32F,outputZeros);
    cv::Mat *opencv_float_out = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32F,outputFloat);
    cv::Mat *opencv_complex_out = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32FC2,outputComplex);
    cv::Mat *opencv_lut = new cv::Mat(256,1,CV_8UC1,lut);
    cv::Mat *opencv_fftw_float_out = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32F,fftwFloat);

    // ----------------------------------
    // Initialize raspberry pi GPU for FFT Performance Tests
    // ----------------------------------
    struct GPU_FFT_TRANS *trans;
    struct GPU_FFT *fft_pass[2];
    int mb = mbox_open();
    unsigned int log2_N  = 10; // 2^10
    unsigned int N  = 1 << 10; //  1<<10 : a 1024x1024 image

    // Malloc / Prepare 1st/2nd FFT pass
    gpu_fft_prepare(mb, log2_N, GPU_FFT_REV, N, fft_pass+0);
    gpu_fft_prepare(mb, log2_N, GPU_FFT_REV, N, fft_pass+1);
    // Transpose from 1st pass output to 2nd pass input
    gpu_fft_trans_prepare(mb, fft_pass[0], fft_pass[1], &trans);

    // ----------------------------------
    // Initialize FFTW/F for FFT Performance Tests
    // ----------------------------------
    unsigned int fftw_threads = 4;
    fftwf_init_threads();
    fftwf_plan_with_nthreads(fftw_threads);
    snprintf(wisdom,255,"/home/pi/ids/fft/plan/SIMD-%d-Thread-%dx%d.plan", fftw_threads, TEST_IMG_WIDTH, TEST_IMG_HEIGHT);
    if( access( wisdom, F_OK ) != -1 ) {
        fftwf_import_wisdom_from_filename(wisdom);
    }
    fftwf_plan plan = fftwf_plan_dft_r2c_2d(TEST_IMG_WIDTH, TEST_IMG_HEIGHT, fftwFloat, fftwComplex, FFTW_MEASURE);
    fftwf_export_wisdom_to_filename(wisdom);

    // ----------------------------------
    // END Initialize raspberry pi GPU for FFT Performance Tests
    // ----------------------------------


    // ----------------------------------
    // Run all Performance Tests
    // ----------------------------------
    run_timed(thresholdTestNaive, "Naive", "Threshold", img, output, len);
    run_timed(thresholdTestNEON, "ARM/NEON","Threshold", img, output, len);
    run_timed(thresholdTestOpenCV, "OpenCV", "Threshold", opencv_input, opencv_output);
    run_timed(lutTestOpenCV, "OpenCV-LUT", "Threshold", opencv_input, opencv_output, opencv_lut);

    run_timed(grayToRGBTestNaive, "Naive", "RGB_Conversion", img, output, len);
    run_timed(grayToRGBTestOpenCV, "OpenCV", "RGB_Conversion", opencv_input, opencv_rgboutput);

    run_timed(grayToFloatTestNaive, "Naive", "FloatConvert", img, outputFloat, len);
    run_timed(grayToFloatTestOpenCV, "OpenCV", "FloatConvert", opencv_input,
              opencv_float_out); // openCV needs extra buffers for temp storage


    run_timed(grayToComplexFloatTestNaive, "Naive", "ComplexConvert", img, outputComplex, len);
    run_timed(grayToComplexFloatTestOpenCV, "OpenCV", "ComplexConvert", opencv_input, opencv_output,
              opencv_float_out, opencv_zeros); // openCV needs extra buffers for temp storage

    run_timed(grayToComplexDFTOpenCV, "OpenCV",   "DFT2d", opencv_input, opencv_float_out, opencv_complex_out);
    run_timed(grayToComplexFFTW, "FFTW/F", "DFT2d", &plan, opencv_input, opencv_fftw_float_out);
    run_timed(complexToComplexGPUFFT, "GPU FFT", "DFT2d", opencv_input, opencv_float_out, opencv_zeros, opencv_complex_out, trans, fft_pass);
    // ----------------------------------
    // END Run all Performance Tests
    // ----------------------------------


    // ----------------------------------
    // CLEANUP
    // ----------------------------------

    gpu_fft_release(fft_pass[0]);
    gpu_fft_release(fft_pass[1]);
    gpu_fft_trans_release(trans);

    fftwf_destroy_plan(plan);
    fftwf_cleanup_threads();

//    free(img);
//    free(output);
//    free(outputRGB);
//    free(outputFloat);
//    free(outputZeros);
//    free(outputComplex);
    fftwf_free(fftwComplex);
    fftwf_free(fftwFloat);

    delete opencv_input;
    delete opencv_output;
    delete opencv_rgboutput;
    delete opencv_zeros;

    delete opencv_float_out;
    delete opencv_complex_out;

    // ----------------------------------
    // END CLEANUP
    // ----------------------------------


    return 0;
}