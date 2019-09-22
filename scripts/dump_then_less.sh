#!/bin/bash
#set -x
SCRIPT_DIR=$( dirname $( realpath $0 ) )
OUTPUT_DIR="$SCRIPT_DIR/../output"
cd $OUTPUT_DIR
./dump_flash
hexdump -C flash_dump.bin | less
cd -
