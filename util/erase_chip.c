#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Erase chip (8 MiB from 000000h - 7FFFFFh) to FFh at an address #######\n");
        fprintf(stderr, "Format: erase_chip\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        if (argc > 1) {
                printf("Too many arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }

        print_usage();

        int fd_spi = spi_init();
        int ret = spi_erase_chip();
        spi_close(fd_spi);

        return ret;
}
