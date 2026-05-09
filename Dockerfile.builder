FROM ubuntu:24.04

ARG TARGETARCH
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    ccache \
    libeigen3-dev \
    libboost-all-dev \
    libasound2-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libimlib2-dev \
    libftdi1-dev \
    && rm -rf /var/lib/apt/lists/*

RUN if [ "$TARGETARCH" = "arm64" ]; then \
      git clone --depth 1 https://github.com/WiringPi/WiringPi.git /tmp/WiringPi && \
      cd /tmp/WiringPi && ./build && cd / && rm -rf /tmp/WiringPi && ldconfig; \
    fi
