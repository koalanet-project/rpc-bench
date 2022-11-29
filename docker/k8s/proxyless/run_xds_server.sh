#!/usr/bin/env bash

# kubectl --kubeconfig ~/nfs/Developing/istio/admin.conf apply -f k8s/rpc-bench-server.yaml
SERVER_POD_NAME=`kubectl --kubeconfig ~/nfs/Developing/istio/admin.conf get pods -l app=rpc-bench-server -ojsonpath='{.items[0].metadata.name}'`
kubectl --kubeconfig ~/nfs/Developing/istio/admin.conf exec -it ${SERVER_POD_NAME} -- env GLOG_minloglevel=2 numactl -N 0 -m 0 /nfs/cjr/Developing/rpc-bench/build/rpc-bench -a throughput -r grpc -d 32 --xds 2>&1
