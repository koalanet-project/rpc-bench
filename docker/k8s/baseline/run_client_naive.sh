#!/usr/bin/env bash

# sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl apply -f k8s/rpc-bench-client.yaml
sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl exec -it rpc-bench-client -- numactl -N 0 -m 0 /nfs/cjr/Developing/rpc-bench/build/rpc-bench -a throughput -r grpc -d 32 --concurrency 32 -p 18000 -t 10 rdma0.danyang-06
