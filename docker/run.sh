#!/bin/bash

docker run --name rpc-bench-box --net=host --hostname rpc-bench-host -it rpc-bench-box zsh

# you have to copy the source code into the container manually
