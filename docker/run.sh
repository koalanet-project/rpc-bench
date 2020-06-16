#!/bin/bash

VOLUME="--volume /mnt:/mnt "
docker run -it --name rpc-bench-box --net=host --hostname rpc-bench-host $VOLUME --cap-add SYS_PTRACE rpc-bench-box zsh

# you have to copy the source code into the container manually
