############################################################
# Dockerfile for Jenkins 
# Based on Ubuntu 20.04
############################################################

FROM ubuntu:20.04
MAINTAINER Jakob Otto

# Disable blocking user input prompt while updating
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin

# Update image and install required packages
RUN apt-get update && apt-get upgrade -y && apt-get autoremove
RUN apt-get install -y \
  gcc-10 \
  g++-10 \
  clang \
  git \
  cmake \
  make \
  libssl-dev \
  vim \
  clang-format \
  gdb

# Add jenkins user
RUN useradd -ms /bin/bash jenkins
USER jenkins
WORKDIR /home/jenkins
