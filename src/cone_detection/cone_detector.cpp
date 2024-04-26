#include "cone_detector.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

#define LEN_CONES 2
#define Y_START 0.55
#define Y_END 0.23

const cv::Scalar LOWER_BLUE = cv::Scalar(100, 100, 0);
const cv::Scalar HIGHER_BLUE = cv::Scalar(140, 255, 255);
const cv::Scalar LOWER_YELLOW = cv::Scalar(16, 0, 143);
const cv::Scalar HIGHER_YELLOW = cv::Scalar(39, 255, 255);

const std::vector<cv::Scalar> LOWER_BOUNDS = { LOWER_BLUE, LOWER_YELLOW };
const std::vector<cv::Scalar> UPPER_BOUNDS = { HIGHER_BLUE, HIGHER_YELLOW };

cv::Point detect_cones(cv::Mat& img) {
    cv::Mat roi = get_roi(img);
    cv::Point mid(roi.cols / 2, roi.rows);
    std::vector<cv::Point> points;

    cv::Point start(img.cols / 2, 0);
    cv::Point end(img.cols / 2, img.rows);

    int thickness = 2;
    int type = cv::LINE_8;

    cv::line(img, start, end, cv::Scalar(255, 0, 0), thickness, type);

    for (int i = 0; i < LEN_CONES; i++) {
        cv::Mat hsv = get_hsv(roi, LOWER_BOUNDS.at(i), UPPER_BOUNDS.at(i));

        cv::Point temp = find_conts(hsv, img);
        std::cout << "Closest point: (" << temp.x << ", " << temp.y << ")" << std::endl;
        points.push_back(temp);
    }

    if ((points[0].x == -1 && points[0].y == -1) && (points[1].x == -1 && points[1].y == -1)) {
        cv::Point pt(-1, -1);
        return pt;
    } else if ((points[0].x != -1 && points[0].y != -1) && (points[1].x == -1 && points[1].y == -1)) {

        draw_circle(img, points[0]);
        return points[0];

    } else if ((points[0].x == -1 && points[0].y == -1) && (points[1].x != -1 && points[1].y != -1)) {

        draw_circle(img, points[1]);
        return points[1];

    } else {
        int d1 = std::abs(mid.x - points[0].x);
        int d2 = std::abs(mid.x - points[1].x);

        if (d1 < d2) {
            draw_circle(img, points[0]);
            return points[0];
        } else {
            draw_circle(img, points[1]);
            return points[1];
        }
    }
}

cv::Mat get_roi(cv::Mat& img) {
    cv::Mat region(img, cv::Rect(0, (int)img.rows * Y_START, img.cols, (int)img.rows * Y_END));
    return region;
}

cv::Mat get_hsv(cv::Mat& img, cv::Scalar lower_bounds, cv::Scalar upper_bounds) {

    cv::Mat hsv_img;
    cv::cvtColor(img, hsv_img, cv::COLOR_BGR2HSV);

    cv::Mat mask;
    cv::inRange(hsv_img, lower_bounds, upper_bounds, mask);

    return mask;
}

cv::Point find_conts(cv::Mat& hsv_roi_img, cv::Mat& og_img) {
    cv::Mat canny_output;
    cv::Canny(hsv_roi_img, canny_output, 50, 150);

    std::vector<std::vector<cv::Point>> contours;
    findContours(canny_output, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    cv::Rect closest;

    // now we have all the contours now we need to draw them and display them
    for (size_t i = 0; i < contours.size(); i++) {

        cv::Rect bounding_rect = cv::boundingRect(contours[i]);

        if (bounding_rect.y > closest.y) {
            closest = bounding_rect;
        }

        // add back the cropped image height to the y coordinate
        bounding_rect.y += (int)og_img.rows * Y_START;
        if (bounding_rect.area() > 400) {
            cv::rectangle(og_img, bounding_rect, cv::Scalar(0, 0, 255), 2, cv::LINE_8);
            cv::Point pt(closest.x + closest.height, closest.y + closest.height);
            return pt;
        }
    }

    cv::Point pt(-1, -1);
    return pt;
}

void draw_circle(cv::Mat& img, cv::Point pt) {
    cv::circle(img,
        pt,
        25,
        cv::Scalar(0, 255, 0),
        cv::FILLED,
        cv::LINE_8);
}