# First stage for building the software:
FROM ubuntu:18.04 as builder

ENV DEBIAN_FRONTEND noninteractive

# Upgrade the Ubuntu 18.04 LTS base image
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y

# Install the development libraries for OpenCV and other dependencies
RUN apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    build-essential \
    libopencv-dev \
    python3 \
    python3-pip \
    python3-setuptools \
    cython

# Install Python packages
RUN pip3 install numpy pandas

# Include this source tree and compile the sources
ADD . /opt/sources
WORKDIR /opt/sources

RUN mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/tmp .. && \
    make && make install

# Second stage for packaging the software into a software bundle:
FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y

RUN apt-get install -y --no-install-recommends \
    libopencv-core3.2 \
    libopencv-highgui3.2 \
    libopencv-imgproc3.2 \
    python3 \
    python3-pip \
    python3-setuptools \
    cython

# Install Python packages
RUN pip3 install cython
RUN pip3 install numpy
RUN pip3 install pandas

WORKDIR /usr/bin
COPY --from=builder /tmp/bin/main .

# Python test stage
WORKDIR /opt/sources
COPY --from=builder /opt/sources .
RUN python3 -m unittest discover -s test

# This is the entrypoint when starting the Docker container; hence, this Docker image is automatically starting our software on its creation
ENTRYPOINT ["/usr/bin/main"]
