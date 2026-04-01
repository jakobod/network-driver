############################################################
# Dockerfile for Jenkins 
# Based on Ubuntu 20.04
############################################################

FROM ubuntu:24.04

# Disable blocking user input prompt while updating
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin

# Update image and install required packages
RUN apt-get update && apt-get install -y apt-utils

# Update image and install required packages
RUN apt-get update && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-14 \
    g++-14 \
    clang-18 \
    clang-format \
    clang-tidy \
    llvm \
    git \
    cmake \
    make \
    ninja-build \
    pkg-config \
    libssl-dev \
    liburing-dev \
    vim \
    nano \
    gdb \
    valgrind \
    curl \
    wget \
    ca-certificates \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Add jenkins user
RUN useradd -ms /bin/bash jenkins
USER jenkins
WORKDIR /home/jenkins
