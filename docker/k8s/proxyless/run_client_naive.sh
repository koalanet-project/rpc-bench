#!/usr/bin/env bash

# sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl apply -f k8s/rpc-bench-client.yaml
SERVER_IP=`sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl get pod rpc-bench-server -o jsonpath='{.status.podIP}'`
echo server_ip: $SERVER_IP
sudo KUBECONFIG=/etc/kubernetes/admin.conf kubectl exec -it rpc-bench-client -- numactl -N 0 -m 0 /nfs/cjr/Developing/rpc-bench/build/rpc-bench -a throughput -r grpc -d 32 --concurrency 32 -p 18000 -t 10 $SERVER_IP
