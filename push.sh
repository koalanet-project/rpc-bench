#!/usr/bin/env bash

WORKDIR=`dirname $(realpath $0)`
cd $WORKDIR

rsync -avP ~/Developing/rpc-bench danyang-01:~/nfs/Developing/ --exclude='deps/*' --exclude='build/*' --exclude='target/*'
