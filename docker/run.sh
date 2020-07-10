#!/bin/bash

VOLUME="--volume /mnt:/mnt "
# docker run -it --name rpc-bench-box --net=host --hostname rpc-bench-host $VOLUME --cap-add SYS_PTRACE --cap-add SYS_ADMIN rpc-bench-box zsh
docker run -it --name rpc-bench-box --net=host --hostname rpc-bench-host $VOLUME --cap-add SYS_PTRACE --cap-add SYS_ADMIN rpc-bench-box bash

# you have to copy the source code into the container manually
