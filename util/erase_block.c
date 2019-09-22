#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Erase block (64 KiB or XX0000h - XXFFFFh) to FFh at an address #######\n");
        fprintf(stderr, "Format: erase_block address\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        if (argc > 2) {
                printf("Too many arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }

        print_usage();

        int addr = strtol(argv[1], NULL, 16);

        int fd_spi = spi_init();

        int ret = spi_erase_block(addr);
        spi_close(fd_spi);
        return ret;
}
