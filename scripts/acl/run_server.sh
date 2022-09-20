#!/usr/bin/env bash

WORKDIR=$(dirname $(realpath $0))
cd $WORKDIR/../../

# Run this on danyang-06 as server
GLOG_minloglevel=2 numactl -N 0 -m 0 ./build/rpc-bench -a hotel_reservation -r grpc -d 32 -C 128 2>&1
