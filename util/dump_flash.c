#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Dumping the whole flash to a file #######\n");
        fprintf(stderr, "Usage: %s [file]\n", "dump_flash");
        fprintf(stderr, "### If no file name is specified, default name \"flash_dump.bin\" will be used ###\n");
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

        int fd_spi = spi_init();

        dump_flash(argc == 2 ? argv[1] : "flash_dump.bin");
        spi_close(fd_spi);

        return 0;
}
