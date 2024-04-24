//encapsualte the functions in order to be callables
#ifndef CONES_DETECTION_H
#define CONES_DETECTION_H

#include <opencv2/opencv.hpp>
cv::Mat detect_cones(cv::Mat& img);
cv::Mat get_roi(cv::Mat& img);
cv::Mat get_hsv(cv::Mat& img,cv::Scalar lower_bounds, cv::Scalar upper_bounds);
void find_conts(cv::Mat& hsv_roi_img,cv::Mat& og_img);

#endif