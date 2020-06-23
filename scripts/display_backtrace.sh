#!/bin/bash
# Usage: scripts/display_backtrace.sh build/rpc-bench /tmp/stderr.log
if [ $# -ne 2 ]; then
	echo "Usage: $0 <entry> <backtrace_file>"
	exit 0
fi
ENTRY=$1
FILE=$2

addr2line -f -C -p -e $ENTRY `cat $FILE | egrep -o '\+0x[^)]+' | tr '\n' ' '`
