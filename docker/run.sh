#!/bin/bash

VOLUME="--volume /nfs:/nfs "
# docker run -it --name rpc-bench-box --net=host --hostname rpc-bench-host $VOLUME --cap-add SYS_PTRACE --cap-add SYS_ADMIN rpc-bench-box zsh
docker run -it --name rpc-bench-box --net=host --hostname rpc-bench-host $VOLUME --cap-add SYS_PTRACE --cap-add SYS_ADMIN --cap-add SYS_NICE rpc-bench-box bash

# you have to copy the source code into the container manually
