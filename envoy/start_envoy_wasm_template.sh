#!/usr/bin/env bash

set -euo pipefail

if [ $# -ne 1 ]; then
	echo "Usage: ./start_envoy_wasm_template.sh [wasm-package-name]"
	echo "Example: ./start_envoy_wasm_template.sh content-based-acl"
	echo "Example: ./start_envoy_wasm_template.sh fault"
	echo "Example: ./start_envoy_wasm_template.sh for-acl"
	exit 0
fi

PKG_NAME=$1
MOD_NAME=${PKG_NAME//-/_}
echo Package name: $PKG_NAME
echo Module name: $MOD_NAME

WORKDIR=`dirname $(realpath $0)`
cd $WORKDIR

pushd ../${PKG_NAME}
cargo build --target=wasm32-unknown-unknown --release
cp target/wasm32-unknown-unknown/release/${MOD_NAME}.wasm /tmp
popd

cat envoy-wasm.yaml | sed "s/content_based_acl/${MOD_NAME}/g" > envoy-wasm-gen.yaml

cp docker-compose.yaml docker-compose-gen.yaml
sed -i "s/envoy-wasm.yaml/envoy-wasm-gen.yaml/g" docker-compose-gen.yaml
sed -i "s/content_based_acl/${MOD_NAME}/g" docker-compose-gen.yaml

sudo docker compose -f docker-compose-gen.yaml up --build
