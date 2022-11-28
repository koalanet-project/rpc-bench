#!/usr/bin/env bash

# sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl apply -f k8s/rpc-bench-client.yaml

# neither IP of the pods or IP of the service works
# <app_name>.<namespace>.svc.cluster.local
SERVER_ADDR=rpc-bench-server.default.svc.cluster.local
sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl exec -it rpc-bench-client -- numactl -N 0 -m 0 /nfs/cjr/Developing/rpc-bench/build/rpc-bench -a throughput -r grpc -d 32 --concurrency 32 -p 18000 -t 10 xds:///$SERVER_ADDR
