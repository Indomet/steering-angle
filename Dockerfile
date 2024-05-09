# First stage for building the software:
    FROM ubuntu:18.04 as builder

    ENV DEBIAN_FRONTEND noninteractive
    
    # Upgrade the Ubuntu 18.04 LTS base image
    RUN apt-get update -y && \
        apt-get upgrade -y && \
        apt-get dist-upgrade -y
    
    # Install the development libraries for OpenCV
    RUN apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        build-essential \
        libopencv-dev \
        python3 \
        python3-pip
    RUN pip3 install numpy pandas
    # Include this source tree and compile the sources
    ADD . /opt/sources
    WORKDIR /opt/sources

    # Python test stage
    WORKDIR /opt/sources
    COPY . .
    RUN python3 -m unittest discover -s test
    RUN python3 test/test.py

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
        libopencv-imgproc3.2 
    
    WORKDIR /usr/bin
    COPY --from=builder /tmp/bin/main .
    # This is the entrypoint when starting the Docker container; hence, this Docker image is automatically starting our software on its creation
    ENTRYPOINT ["/usr/bin/main"]