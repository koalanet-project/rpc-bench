FROM ubuntu:20.04

# grpc dependency requires cmake to be up-to-date as much as possible

ENV DEBIAN_FRONTEND=noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN=true
RUN apt update && apt upgrade -y && \
    apt install -y build-essential cmake g++ gdb git wget zlib1g-dev libssl-dev pkgconf vim-tiny numactl libgoogle-glog-dev libunwind-dev libnuma-dev libsnappy-dev

WORKDIR /root
