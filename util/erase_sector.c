#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Erase sector (4 KiB or XXX000h - XXXFFFh) to FFh at an address #######\n");
        fprintf(stderr, "Format: erase_sector address\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        int addr;
        if (argc > 2) {
                printf("Too many arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }

        print_usage();
        addr = strtol(argv[1], NULL, 16);

        int fd_spi = spi_init();

        int ret = spi_erase_sector(addr);
        spi_close(fd_spi);
        return ret;
}
