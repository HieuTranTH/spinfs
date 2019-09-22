#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Erase a Security Register (256 B or XXXX00h - XXXXFFh) to FFh #######\n");
        fprintf(stderr, "Format: erase_sector address\n");
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

        int ret = spi_erase_sec_reg(addr);
        spi_close(fd_spi);
        return ret;
}
