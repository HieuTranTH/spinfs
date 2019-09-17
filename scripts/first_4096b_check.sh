#!/bin/bash
set -x
SCRIPT_DIR=$( dirname $( realpath $0 ) )
OUTPUT_DIR="$SCRIPT_DIR/../output"
cd $OUTPUT_DIR

./dump_flash lala1; ./dump_flash lala2; hexdump -C lala1 >hex.lala1; hexdump -C lala2 >hex.lala2; vi -d hex.lala1 hex.lala2
cd -
