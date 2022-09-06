#!/usr/bin/env bash

WORKDIR=`dirname $(realpath $0)`
cd $WORKDIR/../../

# Run this on danyang-05 as client
numactl -N 0 -m 0  ./build/rpc-bench -a hotel_reservation -r grpc -d 32 --concurrency 32 -p 18000 -t 10 rdma0.danyang-06
