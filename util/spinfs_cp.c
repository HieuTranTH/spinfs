#if 0
        head = 0x000000;
        tail = 0x000000;
        struct spinfs_raw_inode *root_inode = createInode(NULL, 0);
        //root_inode->name[0] = '/';
        strncpy(root_inode->name, "This_is_the_name_of_root_directory", 32);
        root_inode->inode_num = 255;
        root_inode->parent_inode_num = 0;
        root_inode->data_size = 0;
        root_inode->version = 1;

        printf("Current i-node size: %d bytes.\n", sizeof(*root_inode));
        print_buffer((unsigned char*)root_inode, sizeof(*root_inode));

        int fd_spi = spi_init();
        //spi_write_data(0x020000, (unsigned char*)root_inode, sizeof(*root_inode));
        spi_close(fd_spi);
        free(root_inode);
        return 0;
#endif

#include <sys/stat.h>
#include "spi_flash.h"
#include "spinfs.h"

void print_usage()
{
        fprintf(stderr, "####### Copy files to and from flash, /spinfs/ #######\n");
        fprintf(stderr, "Format: spinfs_cp src dest\n");
        fprintf(stderr, "Append /spinfs/ to tell the file is from flash\n");
        fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
        if (argc != 3) {
                printf("Too many or too less arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        print_usage();

        /*
         * Direction of the transfer
         * 1 - From host to flash
         * 0 - From flash to host
         */
        int t_direct = 1;

        // What if both paths are on flash?
        if (strncmp(argv[1], "/spinfs/", 7) == 0) {
                t_direct = 0;
        } else if (strncmp(argv[2], "/spinfs/", 7) == 0) {
                t_direct = 1;
        } else {
                printf("ERROR: One of the path should be on flash.\n");
                exit(EXIT_FAILURE);
        }
        printf("Direction of the transfer: %d\n", t_direct);

        /*
         * Find source file size
         */
        struct stat info;
        if (t_direct == 1) {
                if (stat(argv[1], &info) == -1) {
                        perror("Stat file error");
                        exit(EXIT_FAILURE);
                }
                printf("File %s has size: %ld\n", argv[1], info.st_size);
        }

        /*
         * Read content of file to buffer
         */
        FILE *fp = fopen(argv[1], "r");
        if (fp == NULL) {
                perror("File open error");
                exit(1);
        }

        // Allocate buffer
        // Maybe dont need to have separated buffer, can read file content to
        // inode.data directly
        unsigned char *buffer = calloc(info.st_size, 1);
        if (buffer == NULL) {
                perror("Allocation error:");
                exit(5);
        }

        //Read file content to buffer
        unsigned char c;
        if (fread(buffer, 1, info.st_size, fp) == 0) {
                perror("EOF error");
                printf("feof return: %d\n", feof(fp));
                printf("ferror return: %d\n", ferror(fp));
        }
        print_buffer(buffer, info.st_size);
        printf("\n");

        //Create spinfs inode with buffer in data
        struct spinfs_raw_inode *next = createInode(buffer, info.st_size);
        printf("Size of the next i-node: %ld\n", sizeof(*next) + info.st_size);
        print_buffer((unsigned char*)next, sizeof(*next) + info.st_size);

        int fd_spi = spi_init();
        spi_write_data(0x030000, (unsigned char*)next, sizeof(*next) + info.st_size);
        spi_close(fd_spi);

        free(next);
        free(buffer);
        fclose(fp);

#if 0
        int addr = strtol(argv[1], NULL, 16);
        int count = argc - 2;

        unsigned char *buffer = calloc(count, sizeof(char));
        if (buffer == NULL) {
                perror("Allocation error:");
                exit(5);
        }

        long int strtol_buf = 0;
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
#endif

        return 0;
}
