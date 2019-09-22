#!/bin/bash
#set -x
SCRIPT_DIR=$( dirname $( realpath $0 ) )
OUTPUT_DIR="$SCRIPT_DIR/../output"

if [ -z $1 ]; then
    echo "Need to give parameter to the script"
    exit 1
fi

for i in $( seq 0 $(( $1 - 1 )) ); do
    #echo $(( $i % 256 ))
    printf "%02x " $(( $i % 256 ))
done
echo
