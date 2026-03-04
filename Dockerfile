# Build Stage
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libeigen3-dev \
    libboost-all-dev \
    libasound2-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libimlib2-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /app
COPY . .

# Build the project
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)


# Runtime Stage
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-thread1.83.0 \
    libboost-log1.83.0 \
    libboost-system1.83.0 \
    libboost-regex1.83.0 \
    libboost-chrono1.83.0 \
    libboost-date-time1.83.0 \
    libboost-filesystem1.83.0 \
    libboost-atomic1.83.0 \
    libboost-program-options1.83.0 \
    libasound2t64 \
    libprotobuf32t64 \
    libimlib2t64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the built server executable and required resources from the builder stage
# Adjust the executable name and paths based on what you actually want to run by default.
COPY --from=builder /app/build/server_simulator/server_simulator /usr/local/bin/server_simulator

# Default entrypoint (can be overridden when running the container)
CMD ["server_simulator"]
