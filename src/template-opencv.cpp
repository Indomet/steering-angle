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
// #include <detect-cones.hpp>
//  Include the OpenDLV Standard Message Set that contains messages that are usually exchanged for automotive or robotic applications
#include "opendlv-standard-message-set.hpp"

// Include the GUI and image processing header files from OpenCV
#include <cone_detection/cone_detector.hpp>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <cmath>

double predict(double speed);

double predict(double speed) {
    std::vector<double> coefficients = {0.00000000e+00, 3.88192460e-03, -2.24144143e-05, -5.07314594e-07, 7.27527660e-09, 3.34496405e-11, -5.53052388e-13};
    double result = 0.0;
    for (int i = 0; i < coefficients.size(); i++) {
        result += coefficients[i] * std::pow(speed, i);
    }
    return result;
}


int32_t main(int32_t argc, char** argv) {
    int totalFrames = 0;
    int total_correct = 0;
    double detectionAccuracy = 0.0;

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
            
            opendlv::proxy::AngularVelocityReading vr;
            std::mutex vMutex;
            auto onGroundSteeringRequest = [&gsr, &gsrMutex](cluon::data::Envelope&& env) {
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                std::cout << "lambda: groundSteering = " << gsr.groundSteering() << std::endl;
            };

            auto onVelocityRequest = [&vr, &vMutex](cluon::data::Envelope &&env)
            {
                // The envelope data structure provide further details, such as sampleTimePoint as shown in this test case:
                // https://github.com/chrberger/libcluon/blob/master/libcluon/testsuites/TestEnvelopeConverter.cpp#L31-L40
                std::lock_guard<std::mutex> lck(vMutex);
                vr = cluon::extractMessage<opendlv::proxy::AngularVelocityReading>(std::move(env));
            };

            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);
            od4.dataTrigger(opendlv::proxy::AngularVelocityReading::ID(), onVelocityRequest);
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
                
                auto sampleTimePoint = sharedMemory->getTimeStamp(); // Get the TimeStamp from shared memory
                int64_t timeMs = cluon::time::toMicroseconds(sampleTimePoint.second); // Get the time in microseconds from the time stamp
                int64_t timeS = cluon::time::toMicroseconds(sampleTimePoint.second) / 1000000; // convert time in microseconds to seconds, and gets rid of the microsecond precision by rounding
                int64_t currentMs = timeMs - timeS * 1000000; // calculates the microseconds that has passed for each second.
                std::string currentMsStr = std::to_string(currentMs); // convert the time in microseconds to string
                std::string timeSStr = std::to_string(timeS); // conver the time in seconds to string

                // cv::Point closest = detect_cones(img);
                std::pair<cv::Point, cv::Point> points = detect_cones(img);
                cv::Point left = points.first;
                cv::Point right = points.second;

                

                cv::Mat roi = get_roi(img);
                cv::Point mid(roi.cols / 2, roi.rows);

                int left_distance = left.x;
                int right_distance = right.x;

                
                //Trig strategy
                int y_bottom = img.rows;
                int leftX = left.x;
                int rightX = right.x;
                std::cout << "leftY: " << left.y << " rightY: " << right.y << "\n";

                int leftY = left.y;
                int rightY = right.y;

                cv::Point center(mid.x, y_bottom);

                int length = 300;

                // Draw threshhold lines
                int leftThreshholdAngle = 135;
                int rightThreshholdAngle = 45;
                cv::Point leftThreshhold(
                    center.x + length * cos(leftThreshholdAngle * M_PI / 180),
                    center.y - length * sin(leftThreshholdAngle * M_PI / 180)
                );
                cv::Point rightThreshhold(
                    center.x + length * cos(rightThreshholdAngle * M_PI / 180),
                    center.y - length * sin(rightThreshholdAngle * M_PI / 180)
                );

                cv::line(img, center, leftThreshhold, cv::Scalar(0,165,255), 2);
                cv::line(img, center, rightThreshhold, cv::Scalar(0,165,255), 2);


               if(gsr.groundSteering() != 0 && gsr.groundSteering() !=-0){
                totalFrames++;
                    
                    
                std::lock_guard<std::mutex> lck(gsrMutex);
                //file << timeSStr << ";" << currentMsStr << ";" << vr.angularVelocityZ() << ";" << gsr.groundSteering() << ";\n";
                double prediction = predict(vr.angularVelocityZ());
                double upper_bound = 1.25 * gsr.groundSteering();
                double lower_bound = 0.75 * gsr.groundSteering();

                if(gsr.groundSteering() < 0){
                    upper_bound = 0.75 * gsr.groundSteering();
                    lower_bound = 1.25 * gsr.groundSteering();
                }

                if (prediction <= upper_bound && prediction >= lower_bound) {
                                total_correct++;
                }
                std::cout << "Accuracy = " << total_correct << "/" << totalFrames << " = " << (double)total_correct / totalFrames << "\n";
                }

                sharedMemory->unlock();

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
