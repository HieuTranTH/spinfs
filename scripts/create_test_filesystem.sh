#!/bin/bash
set -x
SCRIPT_DIR=$( dirname $( realpath $0 ) )
OUTPUT_DIR="$SCRIPT_DIR/../output"
cd $OUTPUT_DIR

./spinfs_mkfs
./spinfs_touch /foo
./spinfs_touch /bar
./spinfs_mkdir /dir1
./spinfs_mkdir /dir1/dir2
./spinfs_touch /dir1/a
./spinfs_touch /dir1/b
./spinfs_touch /dir1/dir2/c
./spinfs_mkdir /dir3
./spinfs_mkdir /dir1/dir4

cd -
