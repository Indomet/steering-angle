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
#include "main.hpp"
#include <string>



double predict(double speed) {
    std::vector<double> coefficients = {0.00000000e+00, 3.96583242e-03, -1.05373477e-04, -3.17407267e-07, 4.68268257e-08, -1.49984238e-10, -5.54462195e-12, 3.47391935e-14};
    double intercept = 0.05159669059756054;
    double prediction = intercept;
    for (int i = 0; i < coefficients.size(); i++) {
        prediction += coefficients[i] * std::pow(speed, i);
    }
    return prediction;
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
                std::lock_guard<std::mutex> lck(gsrMutex);
                gsr = cluon::extractMessage<opendlv::proxy::GroundSteeringRequest>(std::move(env));
                //std::cout << "onGroundSteeringRequest triggered. groundSteering = " << gsr.groundSteering() << std::endl;
            };
            od4.dataTrigger(opendlv::proxy::GroundSteeringRequest::ID(), onGroundSteeringRequest);
            auto onVelocityRequest = [&vr, &vMutex](cluon::data::Envelope&& env) {
                std::lock_guard<std::mutex> lck(vMutex);
                vr = cluon::extractMessage<opendlv::proxy::AngularVelocityReading>(std::move(env));
                //std::cout << "onVelocityRequest triggered. angularVelocityZ = " << vr.angularVelocityZ() << std::endl;
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
                double prediction = predict(vr.angularVelocityZ());
                if (gsr.groundSteering() != 0 && gsr.groundSteering() != -0) {
                    totalFrames++;

                    std::lock_guard<std::mutex> lck(gsrMutex);
                    
                    double upper_bound = 1.25 * gsr.groundSteering();
                    double lower_bound = 0.75 * gsr.groundSteering();

                    if (gsr.groundSteering() < 0) {
                        upper_bound = 0.75 * gsr.groundSteering();
                        lower_bound = 1.25 * gsr.groundSteering();
                    }

                    if (prediction <= upper_bound && prediction >= lower_bound) {
                        total_correct++;
                    }

                    std::cout << "Accuracy = " << total_correct << "/" << totalFrames << " = " << (double)total_correct / totalFrames << "\n";

                    
                }
                std::cout << "group_02;" << timeMs << ";" << prediction << std::endl;
                
    

                sharedMemory->unlock();

                // Display image on your screen.
                if (VERBOSE) {

                    std::string text = "Speed: " + std::to_string(vr.angularVelocityZ()) + " Predicted angle: " + std::to_string(prediction);
                    cv::Point textPosition(10, 30);  

                    // display the text on the image
                    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
                    double fontScale = 0.5;
                    cv::Scalar textColor(0, 0, 255);  // red text
                    cv::putText(img, text, textPosition, fontFace, fontScale, textColor);

                    // show the image
                    cv::imshow(sharedMemory->name().c_str(), img);
                    cv::waitKey(1);
                }
            }
        }
        retCode = 0;
    }

    return retCode;
}
