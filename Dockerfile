############################################################
# Dockerfile for Jenkins 
# Based on Ubuntu 20.04
############################################################

FROM ubuntu:24.04
MAINTAINER Jakob Otto

# Disable blocking user input prompt while updating
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Berlin

# Update image and install required packages
RUN apt update && apt install -y apt-utils

RUN apt upgrade -y && apt autoremove
RUN apt install -y \
  gcc \
  g++ \
  clang \
  git \
  cmake \
  make \
  libssl-dev \
  vim \
  clang-format \
  gdb \
  liburing-dev \
  sudo

# # Add jenkins user
# RUN useradd -ms /bin/bash jenkins
# USER jenkins
# WORKDIR /home/jenkins
