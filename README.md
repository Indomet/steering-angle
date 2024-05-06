## Table of Contents

- [Introduction](#introduction)
  - [Dependencies](#dependencies)
- [Setup](#setup)
  - [Environment](#environment)
  - [Cloning and Building the Project](#cloning-and-building-the-project)
- [Docker Setup](#docker-setup)
  - [Prerequisites](#prerequisites)
  - [Instructions](#instructions)
  - [Notes](#notes)
- [Team Coordination](#team-coordination)
- [License](#license)


# Introduction
## Dependencies
* [Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)
* [CMake](https://cmake.org/)


# Setup
## Environment
This project is based on linux, and is specifically made for Ubuntu-22.04 LTS.

## Cloning and building the project
* This quickstart guide is written with a linux shell in mind. If you use widnows or mac, consider running a virtual machine.
* Make sure you have git installed, otherwise refer to the [official documentation](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)


Make sure your system is up to date and that CMake is installed
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install build-essential cmake git
```

In order to clone the project, open your terminal and paste the following ```git clone git@git.chalmers.se:courses/dit638/students/2024-group-02.git```
Afterwards, you will have the repository cloned.


# Docker Setup

This repository contains Docker commands to set up and run the OpenDLV Vehicle View and H264 Decoder applications alongside a custom OpenCV example. The setup involves running multiple Docker containers, each serving a specific purpose.

## Prerequisites

- Docker installed on your system ([Get Docker](https://docs.docker.com/engine/install/ubuntu/))

## Instructions

1. **Run OpenDLV Vehicle View:**

   Open a terminal and execute the following command:

   ```bash
   docker run --rm -i --init --name=opendlv-vehicle-view -v $PWD:/opt/vehicle-view/recordings -v /var/run/docker.sock:/var/run/docker.sock -p 8081:8081 chrberger/opendlv-vehicle-view:v0.0.64
   ```

   This command starts the OpenDLV Vehicle View Docker container.

2. **Run H264 Decoder:**

   Open another terminal and execute the following command:

   ```bash
   docker run --rm -ti --ipc=host -e DISPLAY=$DISPLAY -v /tmp:/tmp h264decoder:v0.0.5 --cid=253 --name=img --verbose
   ```

   This command runs the H264 Decoder Docker container, enabling it to decode H264 video streams.

3. **Build and Run :**

   First, build the Docker image:

   ```bash
   docker build -f Dockerfile -t my-opencv-example .
   ```
  
   Then, run the container (make sure to process at least 1 frame into the shared memory before running, you can do so by playing a recording while the decoder is running):

   ```bash
   docker run --rm -ti --ipc=host -e DISPLAY=$DISPLAY -v /tmp:/tmp my-opencv-example:latest --cid=253 --name=img --width=640 --height=480 --verbose
   ```


# Team coordination
Each new feature will be introduced by creating an issue that describes the feature with a set of acceptance criteria. Each issue will be assigned to a member to work on where they create a branch that will close the issue upon resolving.

If any unprecedented behaviour is exhibited  by the software, a feature with a bug tag will be created and again, a corresponding branch will be created as well to fix the issue. The code will be verified to have fixed the issue before merging it.

Each merge request needs to be assigned to a team member, and a code review to be conducted before merging

Each commit message needs to be descriptive enough about what was changed followed by the issue number. For example ````git commit -m "added feature #2"```` 

# License
* This project is released under the terms of the GNU GPLv3 License