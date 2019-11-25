#include "spinfs.h"
#include <stdio.h>
#include <stdlib.h>

void print_usage()
{
        fprintf(stderr, "####### Format the flash #######\n");
        fprintf(stderr, "Format: spinfs_mkfs\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        spinfs_init();
        spinfs_format();
        spinfs_deinit();
        return 0;
}
