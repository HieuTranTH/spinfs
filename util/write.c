#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Write some bytes at some address #######\n");
        fprintf(stderr, "Format: write address byte1 byte2 ...\n");
        fprintf(stderr, "byte is written in hex with 2 digits (e.g. ff 1c)\n");
        fprintf(stderr, "\n");
}

char *parse_string_to_hex(char* buf)
{

        return buf;

}

int main(int argc, char *argv[])
{
        int addr, count;
        unsigned char *buffer;
        long int strtol_buf = 0;
        if (argc < 3) {
                printf("Too less arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        print_usage();

        addr = strtol(argv[1], NULL, 16);
        count = argc - 2;

        buffer = calloc(count, sizeof(char));
        if (buffer == NULL) {
                perror("Allocation error:");
                exit(5);
        }
        /*
         * Populate write buffer with the rest of command line parameters
         */
        for (int i = 0; i < count; i++) {
                strtol_buf = strtol(argv[i + 2], NULL, 16);
                buffer[i] = *(char*)&strtol_buf;
        }

        int fd_spi = spi_init();

        int ret = spi_write_data(addr, buffer, count);

        spi_close(fd_spi);
        free(buffer);

        return ret;
}
