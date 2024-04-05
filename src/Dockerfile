# Use a suitable base image for the builder stage
FROM alpine:latest AS builder

# Install necessary packages for building
RUN apk --no-cache add \
    cmake \
    g++ \
    make

# Create a directory for the app
WORKDIR /app

# Copy the entire src directory
COPY src /app/src

# Build the application using static linking and strip debug symbols from the binary
RUN mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static" ../src && \
    make && \
    make test && \
    ctest --rerun-failed --output-on-failure && \
    strip --strip-all helloworld && \
    cp helloworld /tmp

# Use a minimal base image for the final stage
FROM scratch

# Copy the built binary from the builder stage
COPY --from=builder /tmp/helloworld /helloworld

# Set the entry point
ENTRYPOINT ["/helloworld"]
