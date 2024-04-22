/*
 * Copyright (C) 2020  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Include the single-file, header-only middleware libcluon to create high-performance microservices
#include "cluon-complete.hpp"
#include <chrono>
// #include <detect-cones.hpp>
//  Include the OpenDLV Standard Message Set that contains messages that are usually exchanged for automotive or robotic applications
#include "opendlv-standard-message-set.hpp"

// Include the GUI and image processing header files from OpenCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

cv::Mat hsv_filter(cv::Mat& img);
cv::Mat detect_cones(cv::Mat& img);

int32_t main(int32_t argc, char** argv) {
    int32_t retCode { 1 };
    // Parse the command line parameters as we require the user to specify some mandatory information on startup.
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ((0 == commandlineArguments.count("cid")) || (0 == commandlineArguments.count("name")) || (0 == commandlineArguments.count("width")) || (0 == commandlineArguments.count("height"))) {
        std::cerr << argv[0] << " attaches to a shared memory area containing an ARGB image." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OD4 session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:    CID of the OD4Session to send and receive messages" << std::endl;
        std::cerr << "         --name:   name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:  width of the frame" << std::endl;
        std::cerr << "         --height: height of the frame" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=253 --name=img --width=640 --height=480 --verbose" << std::endl;
    } else {
        // Extract the values from the command line parameters
        const std::string NAME { commandlineArguments["name"] };
        const uint32_t WIDTH { static_cast<uint32_t>(std::stoi(commandlineArguments["width"])) };
        const uint32_t HEIGHT { static_cast<uint32_t>(std::stoi(commandlineArguments["height"])) };
        const bool VERBOSE { commandlineArguments.count("verbose") != 0 };

        // Attach to the shared memory.
        std::unique_ptr<cluon::SharedMemory> sharedMemory { new cluon::SharedMemory { NAME } };
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << argv[0] << ": Attached to shared memory '" << sharedMemory->name() << " (" << sharedMemory->size() << " bytes)." << std::endl;

            // Interface to a running OpenDaVINCI session where network messages are exchanged.
            // The instance od4 allows you to send and receive messages.
            cluon::OD4Session od4 { static_cast<uint16_t>(std::stoi(commandlineArguments["cid"])) };

            opendlv::proxy::GroundSteeringRequest gsr;
            std::mutex gsrMutex;
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope&& env) {
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);

            // Endless loop; end the program by pressing Ctrl-C.
            while (od4.isRunning()) {
                // OpenCV data structure to hold an image.
                cv::Mat img;
                // Wait for a notification of a new frame.
                sharedMemory->wait();

                // Lock the shared memory.
                sharedMemory->lock();
                {
                    // Copy the pixels from the shared memory into our own data structure.
                    cv::Mat wrapped(HEIGHT, WIDTH, CV_8UC4, sharedMemory->data());
                    img = wrapped.clone();
                }
                img = hsv_filter(img);
                // TODO: Here, you can add some code to check the sampleTimePoint when the current frame was captured.

                // auto sampleTimePoint = sharedMemory->getTimeStamp();
                sharedMemory->unlock();

                // int64_t timeMs = cluon::time::toMicroseconds(sampleTimePoint.second);
                // std::string timeMsStr = std::to_string(timeMs);

                // auto now = std::chrono::system_clock::now();
                // std::time_t date = std::chrono::system_clock::to_time_t(now);
                // std::tm* dateUTC = std::gmtime(&date);
                // std::stringstream ss;
                // ss << std::put_time(dateUTC, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 format
                // std::string dateStr = ss.str();

                // TODO: Do something with the frame.

                // Example: Draw a red rectangle and display image.

                // cv::rectangle(img, cv::Point(50, 50), cv::Point(100, 100), cv::Scalar(0, 0, 255));
                // cv::putText(img,
                //     "Now: " + dateStr + "; ts: " + timeMsStr + "; Zhao, Zepei",
                //     cv::Point(20, 40), // text position
                //     cv::FONT_HERSHEY_DUPLEX,
                //     0.5, // font size
                //     cv::Scalar(255, 255, 255), // text color
                //     1); // line width

                // If you want to access the latest received ground steering, don't forget to lock the mutex:
                {
                    std::lock_guard<std::mutex> lck(gsrMutex);
                    std::cout << "main: groundSteering = " << gsr.groundSteering() << std::endl;
                }

                // Display image on your screen.
                if (VERBOSE) {
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;
    }

    return retCode;
}

cv::Mat hsv_filter(cv::Mat& img) {
    // Convert the image from BGR to HSV color space
    cv::Mat imgHSV;
    cv::cvtColor(img, imgHSV, CV_BGR2HSV);
    int minHue = 106;
    int maxHue = 138;

    int minSaturation = 100;
    int maxSaturation = 255;

    int minValue = 30;
    int maxValue = 90;
    // Define your min and max HSV values
    cv::Scalar minHSV = cv::Scalar(minHue, minSaturation, minValue);
    cv::Scalar maxHSV = cv::Scalar(maxHue, maxSaturation, maxValue);

    // Create a mask based on the min and max HSV values
    cv::Mat mask;
    cv::inRange(imgHSV, minHSV, maxHSV, mask);

    // Apply the mask to the HSV image
    cv::Mat hsvResult;
    cv::bitwise_and(imgHSV, imgHSV, hsvResult, mask);

    // Split the HSV image into H, S, and V channels
    cv::Mat grayscale;
    cv::cvtColor(hsvResult, grayscale, cv::COLOR_BGR2GRAY);

    // Apply a threshold to the grayscale image
    cv::Mat thresholded;
    double thresholdValue = minValue; // Set this to the desired threshold value
    cv::threshold(grayscale, thresholded, thresholdValue, 255, cv::THRESH_BINARY);

    cv::Mat temp = detect_cones(thresholded);

    return temp;
}

cv::Mat detect_cones(cv::Mat& img) {

    cv::Mat imgCopy = img.clone();
    // Convert imgCopy to BGR color space
    cv::cvtColor(imgCopy, imgCopy, cv::COLOR_GRAY2BGR);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // Define the region of interest (ROI)
    cv::Rect roi(0, img.rows * 0.3, img.cols, img.rows * 0.7);

    // Create a new image from the ROI
    cv::Mat imgROI = img(roi);

    // Find contours in the ROI
    cv::findContours(imgROI, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::cout << "Number of contours found: " << contours.size() << std::endl;

    double minDistance = std::numeric_limits<double>::max(); // Initialize minDistance to a large value
    int closestContourIndex = -1;

    int x = img.cols / 2; // Middle of the image
    int y = img.rows * 0.95; // 95% from the top of the image

    cv::Point targetPoint(x, y); // Define your target point

    for (int i = 0; i < contours.size(); i++) {
        cv::Rect boundingRect = cv::boundingRect(contours[i]);
        cv::Point center = (boundingRect.br() + boundingRect.tl()) * 0.5;
        double distance = cv::norm(targetPoint - center);

        if (distance < minDistance && contours[i].size() > 40) {
            minDistance = distance;
            closestContourIndex = i;
        }
    }

    if (closestContourIndex != -1) {
        cv::Rect boundingRect = cv::boundingRect(contours[closestContourIndex]);
        // Adjust the position of the rectangle based on the ROI
        boundingRect.y += img.rows * 0.3;
        cv::rectangle(imgCopy, boundingRect, cv::Scalar(0, 255, 0), 2);
    }

    /*for (const auto& contour : contours) {
        std::cout << "Size of contour: " << contour.size() << std::endl;

        // Calculate the bounding rectangle for each contour
        if (contour.size() > 35) {
            cv::Rect boundingRect = cv::boundingRect(contour);

            // Draw the bounding rectangle on the original image
            cv::rectangle(imgCopy, boundingRect, cv::Scalar(0, 255, 0), 2);
        }
    }*/
    return imgCopy;
}
