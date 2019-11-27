#include "spi_flash.h"

void print_usage()
{
        fprintf(stderr, "####### Erase a Security Register (256 B or XXXX00h - XXXXFFh) to FFh #######\n");
        fprintf(stderr, "Format: erase_sector register_number\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Too many or too less arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }

        print_usage();

        int addr = strtol(argv[1], NULL, 16);

        int fd_spi = spi_init();
        int ret = 0;

        switch (addr) {
        case 1:
                ret = spi_erase_sec_reg(SEC_REG_1_START_ADDR);
                break;
        case 2:
                ret = spi_erase_sec_reg(SEC_REG_2_START_ADDR);
                break;
        case 3:
                ret = spi_erase_sec_reg(SEC_REG_3_START_ADDR);
                break;
        default:
                fprintf(stderr, "Wrong Security Register number!\n");
                ret = EXIT_FAILURE;
        }

        spi_close(fd_spi);
        return ret;
}
