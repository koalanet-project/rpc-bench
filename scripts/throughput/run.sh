#!/usr/bin/env bash

for i in {0..10}; do
    echo "Run ${i}"
    python3 bench_tput.py
done
