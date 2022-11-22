#!/usr/bin/env bash

# kubectl --kubeconfig ~/nfs/Developing/istio/admin.conf apply -f k8s/rpc-bench-server.yaml
kubectl --kubeconfig ~/nfs/Developing/istio/admin.conf exec -it rpc-bench-server -- env GLOG_minloglevel=2 numactl -N 0 -m 0 /nfs/cjr/Developing/rpc-bench/build/rpc-bench -a throughput -r grpc -d 32 2>&1
