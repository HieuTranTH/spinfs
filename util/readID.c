#include "spi_flash.h"

void print_usage()
{
        printf("####### Print Device Identification Info #######\n");
        printf("Format: 00 {Manufacture ID} {Device Type} {Capacity}\n");
        printf("### Should return: [ 00 01 40 17 ] ###\n\n");
}

int main(int argc, char *argv[])
{
        print_usage();

        int fd_spi = spi_init();

        unsigned char buf[] = {0x9f, 0, 0 ,0};
        int ret = wiringPiSPIDataRW(SPI_CHANNEL, buf, sizeof(buf));
        printf("%02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);

        return (ret != sizeof(buf)) ? ret : 0;
}
