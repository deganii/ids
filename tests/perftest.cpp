//
// Created by idegani on 7/3/2019.
//

// Compare OpenCV running on OpenCL vs straight-C vs ARM NEON
#include <iostream>
#include <chrono>
#include <vector>



#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/opencv.hpp>


#include "imgproc.h"
//#include <sys/time.h>
#include <time.h>
#include <gpu_fft.h>

#define TEST_IMG_WIDTH  1024
#define TEST_IMG_HEIGHT 768
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
            printf("%-12s %-12s %5.2fms Calculated over %06d iterations | Total time: %5.2fs\n",
                   method, operation, (float)execution_time_msec / TEST_ITERATIONS,
                   TEST_ITERATIONS,  (float)execution_time_msec/1000.0);
            return (stop - start) / TEST_ITERATIONS;
};




int main(){
    int len = TEST_IMG_WIDTH*TEST_IMG_HEIGHT;
    char *img = getTestImage(TEST_IMG_WIDTH, TEST_IMG_HEIGHT, TEST_IMG_DEPTH);
    char *output = (char *)malloc(len);
    char *outputRGB = (char *)malloc(len*TEST_RGB_DEPTH);
    char *lut = (char *)calloc(256,sizeof(char));

    float *outputFloat = (float *)malloc(len*sizeof(float));
    float *outputZeros = (float *)malloc(len*sizeof(float));
    GPU_FFT_COMPLEX *outputComplex = (GPU_FFT_COMPLEX *)malloc(len*sizeof(GPU_FFT_COMPLEX));

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

    run_timed(thresholdTestNaive, "Naive", "Threshold", img, output, len);
    run_timed(thresholdTestNEON, "ARM/NEON","Threshold", img, output, len);
    run_timed(thresholdTestOpenCV, "OpenCV", "Threshold", opencv_input, opencv_output);
    run_timed(lutTestOpenCV, "OpenCV-LUT", "Threshold", opencv_input, opencv_output, opencv_lut);
//
//    run_timed(grayToRGBTestNaive, "Naive", "RGB_Conversion", img, output, len);
//    run_timed(grayToRGBTestOpenCV, "OpenCV", "RGB_Conversion", opencv_input, opencv_rgboutput);
//
//    run_timed(grayToFloatTestNaive, "Naive", "FloatConvert", img, outputFloat, len);
//    run_timed(grayToFloatTestOpenCV, "OpenCV", "FloatConvert", opencv_input,
//              opencv_float_out); // openCV needs extra buffers for temp storage
//
//
//    run_timed(grayToComplexFloatTestNaive, "Naive", "ComplexConvert", img, outputComplex, len);
//    run_timed(grayToComplexFloatTestOpenCV, "OpenCV", "ComplexConvert", opencv_input, opencv_output,
//              opencv_float_out, opencv_zeros); // openCV needs extra buffers for temp storage



    run_timed(grayToComplexDFTOpenCV, "OpenCV",   "DFT2d", opencv_input, opencv_float_out, opencv_complex_out);
    run_timed(grayToComplexDFTOpenCV, "ARM/NEON", "DFT2d", opencv_input, opencv_float_out, opencv_complex_out);


    free(img);
    free(output);
    free(outputRGB);
    free(outputFloat);
    free(outputZeros);
    free(outputComplex);

    delete opencv_input;
    delete opencv_output;
    delete opencv_rgboutput;

    delete opencv_float_out;
    delete opencv_complex_out;

    return 0;
}