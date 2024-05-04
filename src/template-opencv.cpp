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
#include <cmath>
#include <cone_detection/cone_detector.hpp>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

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
                auto sampleTimePoint = sharedMemory->getTimeStamp(); // Get the TimeStamp from shared memory
                int64_t timeMs = cluon::time::toMicroseconds(sampleTimePoint.second); // Get the time in microseconds from the time stamp
                int64_t timeS = cluon::time::toMicroseconds(sampleTimePoint.second) / 1000000; // convert time in microseconds to seconds, and gets rid of the microsecond precision by rounding
                int64_t currentMs = timeMs - timeS * 1000000; // calculates the microseconds that has passed for each second.
                std::string currentMsStr = std::to_string(currentMs); // convert the time in microseconds to string
                std::string timeSStr = std::to_string(timeS); // conver the time in seconds to string

                // cv::Point closest = detect_cones(img);

                std::pair<cv::Point, cv::Point> points = detect_cones(img);
                // points.first is the blue cone, points.second is the yellow cone
                cv::Point left = points.second;
                cv::Point right = points.first;

                cv::Mat roi = get_roi(img);
                cv::Point mid(roi.cols / 2, roi.rows);

                int left_distance = std::abs(left.x - mid.x);
                int right_distance = std::abs(right.x - mid.x);

                // Trig strategy
                int y_bottom = img.rows;

                int leftX = img.cols - left.x; // flip the x-coordinates
                int rightX = right.x;
                std::cout << "leftY: " << left.y << " rightY: " << right.y << "\n";

                // if (leftX < mid.x) {
                //     leftX = leftX * -1;
                // }
                // if (rightX < mid.x) {
                //     rightX = rightX * -1;
                // }

                cv::Point center(mid.x, y_bottom);

                int leftY = center.y - left.y; // Negate the y-coordinates
                int rightY = center.y - right.y; // Negate the y-coordinates

                int length = 300;

                // Compute the tangent of the angle
                double right_angleRadians = atan2(rightY, rightX);
                double left_angleRadians = atan2(leftY, leftX);

                cv::Point leftEnd(
                    static_cast<int>(center.x + length * cos(left_angleRadians)),
                    static_cast<int>(center.y - length * sin(left_angleRadians)));
                cv::Point rightEnd(
                    static_cast<int>(center.x + length * cos(right_angleRadians)),
                    static_cast<int>(center.y - length * sin(right_angleRadians)));

                leftEnd.x = img.cols - leftEnd.x;
                // cv::line(img, center, leftEnd, cv::Scalar(255, 0, 0), 2);
                cv::line(img, center, leftEnd, cv::Scalar(0, 0, 255), 2);
                cv::line(img, center, rightEnd, cv::Scalar(0, 255, 0), 2);

                // Draw threshhold lines
                int threshold = 50;
                int leftThreshholdAngle = 90 + threshold;
                int rightThreshholdAngle = 90 - threshold;
                double leftThreshholdAngleRadians = leftThreshholdAngle * M_PI / 180;
                double rightThreshholdAngleRadians = rightThreshholdAngle * M_PI / 180;
                cv::Point leftThreshhold(
                    static_cast<int>(center.x + length * cos(leftThreshholdAngleRadians)),
                    static_cast<int>(center.y - length * sin(leftThreshholdAngleRadians)));
                cv::Point rightThreshhold(
                    static_cast<int>(center.x + length * cos(rightThreshholdAngleRadians)),
                    static_cast<int>(center.y - length * sin(rightThreshholdAngleRadians)));

                cv::line(img, center, leftThreshhold, cv::Scalar(0, 165, 255), 2);
                cv::line(img, center, rightThreshhold, cv::Scalar(0, 165, 255), 2);

                std::cout << "Left angle: " << left_angleRadians << " Right angle: " << right_angleRadians << "\n";
                std::cout << "Left threshold: " << leftThreshholdAngleRadians << " Right threshold: " << rightThreshholdAngleRadians << "\n";

                bool shouldTurnLeft = right.x != -1 && right_angleRadians >= rightThreshholdAngleRadians;
                bool shouldTurnRight = left.x != -1 && M_PI - left_angleRadians <= leftThreshholdAngleRadians;

                if (shouldTurnLeft && shouldTurnRight) {
                    // Both conditions are met, decide what to do (e.g., stop, go straight, choose based on which angle is larger, etc.)
                    cv::putText(img, "Decision Needed", cv::Point(center.x - 50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                } else if (shouldTurnLeft) {
                    cv::putText(img, "Turn Left", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                } else if (shouldTurnRight) {
                    cv::putText(img, "Turn Right", cv::Point(300, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                } else {
                    cv::putText(img, "Go Straight", cv::Point(center.x - 50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
                }

                std::string filename = "output.csv";

                // Check if file is empty
                std::ifstream inFile(filename);
                bool isEmpty = inFile.peek() == std::ifstream::traits_type::eof();
                inFile.close();

                // Open file in append mode
                std::ofstream file(filename, std::ios_base::app);

                if (file.is_open()) {
                    // If file was empty, write the headers
                    if (isEmpty) {
                        file << "seconds;microseconds;left_x;right_x;groundsteering;\n";
                    }

                    {
                        std::lock_guard<std::mutex> lck(gsrMutex);
                        file << timeSStr << ";" << currentMsStr << ";" << left_distance << ";" << right_distance << ";" << gsr.groundSteering() << ";\n";
                    }
                    file.close();
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
