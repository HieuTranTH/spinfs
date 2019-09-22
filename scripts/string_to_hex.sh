#!/bin/bash
#set -x
SCRIPT_DIR=$( dirname $( realpath $0 ) )
OUTPUT_DIR="$SCRIPT_DIR/../output"

echo -n "$@" | hexdump -v -e '/1 "%02X "' ; echo
