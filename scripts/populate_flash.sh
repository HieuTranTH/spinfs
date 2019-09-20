#!/bin/bash
set -x
SCRIPT_DIR=$( dirname $( realpath $0 ) )
OUTPUT_DIR="$SCRIPT_DIR/../output"
cd $OUTPUT_DIR

./write 0x000000 53 74 61 72 74 5F 6F 66 5F 4D 65 6D 6F 72 79
./write 0x7ffff3 45 6E 64 5F 6F 66 5F 4D 65 6D 6F 72 79
./write 0x001500 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
./write 0x00508a 43 6F 75 6E 74 65 72 20 53 74 72 69 6B 65 3A 20 47 6C 6F 62 61 6C 20 4F 66 66 65 6E 73 69 76 65
./write 0x1ffbb1 44 65 66 65 6E 73 65 20 6F 66 20 74 68 65 20 41 6E 63 69 65 6E 74 73
./write 0x282651 47 68 6F 73 74 20 54 72 69 63 6B
./write 0x5bac20 57 65 73 74 65 6E 64 69 6E 74 69 65 20 31
./write 0x6ffa00 45 73 70 6F 6F 2C 20 46 69 6E 6C 61 6E 64
./write 0x700000 fe dc ba 98 76 54 32 10 10 23 45 67 89 ab cd ef
./write 0x000800 55 62 75 6E 74 75 0A
./write 0x001000 47 4E 4F 4D 45 0A
./write 0x001400 55 6E 69 74 79 0A
./write 0x0007f0 62 65 66 6F 72 65 20 55 62 75 6E 74 75
./write 0x000ff0 62 65 66 6F 72 65 20 47 4E 4F 4D 45
./write 0x0013f0 62 65 66 6F 72 65 20 55 6E 69 74 79
cd -
