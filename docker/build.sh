#!/bin/bash

docker build --pull -t rpc-bench-box --build-arg SSH_PUBKEY=sshkey.pub .
