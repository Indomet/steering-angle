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
#include <vector>
cv::Mat detect_cones(cv::Mat& img);
cv::Mat get_roi(cv::Mat& img);
cv::Mat get_hsv(cv::Mat& img,cv::Scalar lower_bounds, cv::Scalar upper_bounds);
cv::Mat find_conts(cv::Mat& hsv_roi_img,cv::Mat& og_img);

#define LEN_CONES 2

const cv::Scalar LOWER_BLUE = cv::Scalar(88,76,0);
const cv::Scalar HIGHER_BLUE = cv::Scalar(141,255,255);
const cv::Scalar LOWER_YELLOW = cv::Scalar(15,100,68);
const cv::Scalar HIGHER_YELLOW = cv::Scalar(30,255,255);


const std::vector <cv::Scalar> LOWER_BOUNDS = {LOWER_BLUE,LOWER_YELLOW};
const std::vector <cv::Scalar> UPPER_BOUNDS = {HIGHER_BLUE,HIGHER_YELLOW};

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
                img = detect_cones(img);

                // auto sampleTimePoint = sharedMemory->getTimeStamp();
                sharedMemory->unlock();


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


cv::Mat detect_cones(cv::Mat& img){

    for (int i=0; i < LEN_CONES; i++){
        cv::Mat roi = get_roi(img);
        cv::Mat hsv = get_hsv(roi,LOWER_BOUNDS.at(i),UPPER_BOUNDS.at(i));
        img = find_conts(hsv,img);
    }
    return img;

}
cv::Mat get_roi(cv::Mat& img){
    cv::Mat region (img, cv::Rect(0,(int)img.rows*0.55,img.cols,(int)img.rows*0.28));
    return region;
}


cv::Mat get_hsv(cv::Mat& img,cv::Scalar lower_bounds, cv::Scalar upper_bounds){

    cv::Mat hsv_img;
    cv::cvtColor(img,hsv_img,cv::COLOR_BGR2HSV);

    cv::Mat mask;
    cv::inRange(hsv_img,lower_bounds,upper_bounds,mask);

    return mask;
}

cv::Mat find_conts(cv::Mat& hsv_roi_img,cv::Mat& og_img){
    cv::Mat canny_output;
    cv::Canny(hsv_roi_img, canny_output, 50, 150 );

    std::vector<std::vector<cv::Point> > contours;
    findContours(canny_output, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE );

    //now we have all the contours now we need to draw them and display them
    for(size_t  i=0; i< contours.size(); i++){
        cv::Rect bounding_rect = cv::boundingRect(contours[i]);
        //add back the cropped image height to the y coordinate
        bounding_rect.y += (int)og_img.rows*0.5;

        cv::rectangle(og_img,bounding_rect,cv::Scalar(0,0,255),2,cv::LINE_8);
        
    }
    return og_img;
    
}