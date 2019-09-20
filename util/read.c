#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Read some bytes at some address #######\n");
        fprintf(stderr, "Format: read [address] [bytes]\n");
        fprintf(stderr, "### address default = 0x000000 ###\n");
        fprintf(stderr, "### bytes default   = 1        ###\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        int addr, count;
        if (argc > 3) {
                printf("Too many arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        else if (argc == 3) {
                addr = strtol(argv[1], NULL, 16);
                count = atoi(argv[2]);
        }
        else if (argc == 2) {
                addr = 0x000000;
                count = atoi(argv[1]);
        }
        else if (argc == 1) {
                addr = 0x000000;
                count = 1;
        }
        print_usage();

        unsigned char *buffer = malloc(count * sizeof(*buffer));
        if (buffer == NULL) {
                perror("Allocation error:");
                exit(5);
        }

        int fd_spi = spi_init();

        int ret = spi_read_data(addr, buffer, count);

        printf("Data sequence of %d byte(s) at address %06x is:\n", count, addr);
        print_buffer(buffer, count);

        spi_close(fd_spi);
        free(buffer);

        return ret;
}
