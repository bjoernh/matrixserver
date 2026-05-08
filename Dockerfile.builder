FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libeigen3-dev \
    libboost-all-dev \
    libasound2-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libimlib2-dev \
    && rm -rf /var/lib/apt/lists/*
