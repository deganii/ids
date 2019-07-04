//
// Created by idegani on 7/3/2019.
//

// Compare OpenCV running on OpenCL vs straight-C vs ARM NEON

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc_c.h"
#include <opencv2/opencv.hpp>

//#include <opencv/cv.hpp>

#include "imgproc.h"
//#include <sys/time.h>
#include <time.h>
#include <gpu_fft.h>

#define TEST_IMG_WIDTH  1024
#define TEST_IMG_HEIGHT 768
#define TEST_IMG_DEPTH  1
#define TEST_RGB_DEPTH  3
#define TEST_ITERATIONS 100

void diff(struct timespec *start, struct timespec *stop,
          struct timespec *result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

char *getTestImage(int width, int height, int bytes_per_pixel){

    // native approach

    char *img =  (char *)malloc(width*height*bytes_per_pixel);
    for(int i = 0; i < width*height; i++ ){
        img[i] = (char)i;
    }
    return img;


}

void fftTest(){

}

void grayToRGBTestOpenCV(char *img, char *output, int len){
    // OpenCV threshold
    cv::cvtColor((cv::Mat)(*(cv::Mat*)img), (cv::Mat)(*(cv::Mat*)output), cv::COLOR_GRAY2RGB);
}

void grayToRGBTestNEON(char *img, char *output, int len){
    // ARMV7 Optimized threshold
    computeThresholdNEON(img,output,len,127,255);
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


void grayToComplexFloatTestNaive(char *img, char *output, char *empty1, char *empty2,int len){
    // naive complex float conversion
    for(int r2c = 0; r2c < len; r2c++){
        ((GPU_FFT_COMPLEX *)output)[r2c].re = (float)img[r2c];
    }
}

void grayToComplexFloatTestOpenCV(char *img, char *output, char *floats, char *zero, int len){
    cv::Mat *input = (cv::Mat*)img;
    cv::Mat *cv_output = (cv::Mat*)output;
    cv::Mat *cv_zeros = (cv::Mat*)zero;
    cv::Mat *cv_floats = (cv::Mat*)floats;
    cv::Mat planes[] = {*cv_floats, *cv_zeros};
    // opencv complex float conversion
    input -> convertTo(*cv_floats, CV_32F);
    cv::merge(planes, 2, *cv_output);
}



void grayToComplexDFTOpenCV(char *img, char *output, char *floats, char *zero, int len){
    cv::Mat *input = (cv::Mat*)img;
    cv::Mat *cv_output = (cv::Mat*)output;
    cv::Mat *cv_zeros = (cv::Mat*)zero;
    cv::Mat *cv_floats = (cv::Mat*)floats;
    cv::Mat planes[] = {*cv_floats, *cv_zeros};
    // opencv complex float conversion
    input -> convertTo(*cv_floats, CV_32F);
    cv::merge(planes, 2, *cv_output);
    //cv:dft()
}




void thresholdTestOpenCV(char *img, char *output, int len){
    // OpenCV threshold
    cv::threshold((cv::Mat)(*(cv::Mat*)img), (cv::Mat)(*(cv::Mat*)output), 127, 255, cv::ThresholdTypes::THRESH_BINARY );
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

void run_dft_timed(void (*dft_func)(char *, char *, char *, char *, int), char*name, char *img, char *output,
                   char *floats, char *zeros,  int len, int num_iter){
    struct timespec start, finish, diff_t;

    clock_gettime( CLOCK_MONOTONIC, &start );
    for(int i = 0; i < num_iter; i++){
        dft_func(img, output, floats, zeros, len);
    }
    clock_gettime( CLOCK_MONOTONIC, &finish );
    diff(&start, &finish, &diff_t);
    float diff_secs = diff_t.tv_sec + (diff_t.tv_nsec / 1.0e9);
    printf("%s: %02.2fms, Calculated over %06d iterations, Total time: %02.2fs\n",
           name, 1000.0* diff_secs / num_iter, num_iter, diff_secs);

}


void run_timed(void (*thresh)(char *, char *, int), char*name, char *img, char *output, int len, int num_iter){
    struct timespec start, finish, diff_t;

    clock_gettime( CLOCK_MONOTONIC, &start );
    for(int i = 0; i < num_iter; i++){
        thresh(img, output, len);
    }
    clock_gettime( CLOCK_MONOTONIC, &finish );
    diff(&start, &finish, &diff_t);
    float diff_secs = diff_t.tv_sec + (diff_t.tv_nsec / 1.0e9);
    printf("%s: %02.2fms, Calculated over %06d iterations, Total time: %02.2fs\n",
           name, 1000.0* diff_secs / num_iter, num_iter, diff_secs);

}


int main(){
    int len = TEST_IMG_WIDTH*TEST_IMG_HEIGHT;
    char *img = getTestImage(TEST_IMG_WIDTH, TEST_IMG_HEIGHT, TEST_IMG_DEPTH);
    char *output = (char *)malloc(len);
    char *outputRGB = (char *)malloc(len*TEST_RGB_DEPTH);

    char *outputFloat = (char *)malloc(len*sizeof(float));
    char *outputZeros = (char *)malloc(len*sizeof(float));
    char *outputComplex = (char *)malloc(len*sizeof(GPU_FFT_COMPLEX));


    cv::Mat *opencv_input = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_8UC1,img);
    cv::Mat *opencv_output = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_8UC1,output);
    cv::Mat *opencv_rgboutput = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_8UC3,outputRGB);
    cv::Mat *opencv_zeros = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32F,outputZeros);
    cv::Mat *opencv_float_out = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32F,outputFloat);
    cv::Mat *opencv_complex_out = new cv::Mat(TEST_IMG_WIDTH,TEST_IMG_HEIGHT,CV_32FC2,outputComplex);

    //grayToComplexFloatTestOpenCV((char*)opencv_input, (char*)opencv_complex_out, (char*)opencv_float_out, (char*)opencv_zeros, -1);

    run_dft_timed(grayToComplexFloatTestNaive,
            (char*)"Naive    ComplexConvert", img, outputComplex,
                  NULL, NULL, len, 1000);
    run_dft_timed(grayToComplexFloatTestOpenCV,  (char*)"OpenCV    ComplexConvert",
            (char*)opencv_input, (char*)opencv_output,
            (char*)opencv_float_out, (char*)opencv_zeros, len, 1000);


//    printf("Img: ");
//    for(int i = 0; i < 10; i++){
//        printf("%i ", img[i]);
//    }
//    printf("\n");
//
//    printf("Float: ");
//    for(int i = 0; i < 10; i++){
//        printf("%f ", ((float *)outputComplex)[i]);
//    }
//    printf("\n");



    run_timed(thresholdTestNaive,  (char*)"Naive    Threshold", img, output, len, 1000);
    run_timed(thresholdTestNEON,   (char*)"ARM/NEON Threshold", img, output, len, 1000);
    run_timed(thresholdTestOpenCV, (char*)"OpenCV   Threshold", (char *)opencv_input,(char *)opencv_output, len, 1000);

    run_timed(grayToRGBTestNaive,  (char*)"Naive   RGB_Conversion", img, output, len, 1000);
    run_timed(grayToRGBTestOpenCV, (char*)"OpenCV  RGB_Conversion", (char *)opencv_input,(char *)opencv_rgboutput, len, 1000);





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