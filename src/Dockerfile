# Stage 1: Build the application using multyi stage build as this article says its efficent  https://learnk8s.io/blog/smaller-docker-images
#debian is a minimal image that further reduces the size of the final image
#the image was found by googling "smallest base image docker debina slim" then i change the image from here https://hub.docker.com/_/debian
FROM debian:buster-slim as builder
#update and upgrade packages then remove the package list files to reduce the image size
RUN apt-get update && \
    apt-get install -y \
        cmake \
        g++ \
        make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/sources
COPY . .
#create a build directory to run cmake and add a compiler flag to statically link the code
# then the resulting binary is copied to the /tmp directory and then we strip symbols from the binary to reduce size
RUN mkdir build \
    && cd build \
    && cmake -D CMAKE_BUILD_TYPE=MinSizeRel -D CMAKE_EXE_LINKER_FLAGS=-static .. \
    && make \
    && make test \
    && cp helloworld /tmp \
    && strip --strip-all /tmp/helloworld

# Stage 2: Create a lightweight final image
FROM scratch

# Copy only necessary runtime dependencies from the builder stage
COPY --from=builder /tmp/helloworld /helloworld

ENTRYPOINT ["/helloworld"]