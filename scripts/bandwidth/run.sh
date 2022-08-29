#!/usr/bin/env bash

python3 bench_bw.py rdma0.danyang-06 --range 1,14 -c 32
python3 bench_bw.py rdma0.danyang-06 --range 1,14 -c 1
