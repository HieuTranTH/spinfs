#include "spi_flash.h"

void print_usage()
{
        printf("####### Read some bytes at some address #######\n");
        printf("Format: read [address] [bytes]\n");
        printf("### address default = 0x000000 ###\n");
        printf("### bytes default   = 1        ###\n\n");
}

int main(int argc, char *argv[])
{
        int addr, count;
        if (argc > 3) {
                printf("Too many arguments\n\n");
                print_usage();
                exit(1);
        }
        else if (argc == 3) {
                addr = strtol(argv[1], NULL, 16);
                count = atoi(argv[2]);
        }
        else if (argc == 2) {
                addr = strtol(argv[1], NULL, 16);
                count = 1;
        }
        else if (argc == 1) {
                addr = 0x000000;
                count = 1;
        }
        print_usage();

        unsigned char *buff;
        buff = (unsigned char*)malloc(4 * sizeof(*buff));

        //printf("0x%06x %d\n", addr, count);
        //exit(0);

        int fd_spi = spi_init();

        int ret = spi_read_data(addr, buff, count, 1);

        close(fd_spi);
        free(buff);

        return ret;
}
