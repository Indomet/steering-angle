#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include "cone_detector.hpp"

#define LEN_CONES 2
#define Y_START 0.55
#define Y_END 0.23

const cv::Scalar LOWER_BLUE = cv::Scalar(88,76,0);
const cv::Scalar HIGHER_BLUE = cv::Scalar(141,255,255);
const cv::Scalar LOWER_YELLOW = cv::Scalar(15,100,68);
const cv::Scalar HIGHER_YELLOW = cv::Scalar(30,255,255);


const std::vector <cv::Scalar> LOWER_BOUNDS = {LOWER_BLUE,LOWER_YELLOW};
const std::vector <cv::Scalar> UPPER_BOUNDS = {HIGHER_BLUE,HIGHER_YELLOW};


cv::Mat detect_cones(cv::Mat& img){
    cv::Mat roi = get_roi(img);

    for (int i=0; i < LEN_CONES; i++){
        cv::Mat hsv = get_hsv(roi,LOWER_BOUNDS.at(i),UPPER_BOUNDS.at(i));
        find_conts(hsv,img);
    }
    return img;

}
cv::Mat get_roi(cv::Mat& img){
    cv::Mat region (img, cv::Rect(0,(int)img.rows*Y_START,img.cols,(int)img.rows*Y_END));
    return region;
}


cv::Mat get_hsv(cv::Mat& img,cv::Scalar lower_bounds, cv::Scalar upper_bounds){

    cv::Mat hsv_img;
    cv::cvtColor(img,hsv_img,cv::COLOR_BGR2HSV);

    cv::Mat mask;
    cv::inRange(hsv_img,lower_bounds,upper_bounds,mask);

    return mask;
}

void find_conts(cv::Mat& hsv_roi_img,cv::Mat& og_img){
    cv::Mat canny_output;
    cv::Canny(hsv_roi_img, canny_output, 50, 150 );

    std::vector<std::vector<cv::Point> > contours;
    findContours(canny_output, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE );

    cv::Rect closest;

    //now we have all the contours now we need to draw them and display them
    for(size_t  i=0; i< contours.size(); i++){
      
        cv::Rect bounding_rect = cv::boundingRect(contours[i]);
        if(closest.area() == 0){
         closest=bounding_rect; 
        }

        else if(bounding_rect.y > closest.y){
          closest=bounding_rect; 
        }



        //add back the cropped image height to the y coordinate
        bounding_rect.y += (int)og_img.rows*Y_START;
        if(bounding_rect.area() > 750){cv::rectangle(og_img,bounding_rect,cv::Scalar(0,0,255),2,cv::LINE_8);}
        
    }

}

