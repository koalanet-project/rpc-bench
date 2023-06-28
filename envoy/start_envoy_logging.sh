#!/usr/bin/env bash

set -euo pipefail

WORKDIR=`dirname $(realpath $0)`
cd $WORKDIR

cp docker-compose.yaml docker-compose-gen.yaml
sed -i "s/envoy-wasm.yaml/envoy-logging.yaml/g" docker-compose-gen.yaml
sed -i '/.*content_based_acl.*/d' docker-compose-gen.yaml

sudo docker compose -f docker-compose-gen.yaml up --build
