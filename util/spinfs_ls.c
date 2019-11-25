#include "spinfs.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>           // stat

void print_usage()
{
        fprintf(stderr, "####### Copy files to and from flash, /spinfs/ #######\n");
        fprintf(stderr, "Format: spinfs_cp src dest\n");
        fprintf(stderr, "Append /spinfs/ to tell the file is from flash\n");
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

        spinfs_init();

        char *path = argv[1];
        int pathlen = strlen(path);

        uint32_t target_inode_num = 1;  //start traversing from root directory
        if (path[0] != '/') {
                printf("Path needs to be absolute!\n");
                exit(EXIT_FAILURE);
        }
        printf("\npath: %s\n", path);
        printf("pathlen: %d\n\n", pathlen);

        printf("Inode %d, addr 0x%06x, ver %d.\n", target_inode_num, spinfs_get_inode_table_entry(target_inode_num).physical_addr, spinfs_get_inode_table_entry(target_inode_num).version);
        /*
         * Parse the path to get the target i-node
         */
        struct spinfs_raw_inode *s = malloc(sizeof(*s));
        char *token;
        token = strtok(path, "/");      //TODO will modify argv[1]
        if (token == NULL) {
                // target i-node is root directory
                target_inode_num = 1;
        } else {
                // TODO get current directory name from path
                while (token != NULL) {
                        s = spinfs_read_inode(s,
                                spinfs_get_inode_table_entry(target_inode_num).physical_addr);
                        target_inode_num = find_file_in_dir(s, token);
                        if (target_inode_num == 0) break;

                        token = strtok(NULL, "/");
                }
        }
#if 0


        /*
         * List content of the i-node afer path processing
         * TODO if i-node is a directory, list content; otherwise repeat its name
         */
        if (target_inode_num == 0) {
                printf("File/Directory not found.\n");
        } else {
                get_inode_at_addr(&s, fp, itable[target_inode_num].physical_addr);
                if (S_ISREG(s->mode)) {        // target is a regular file
                        ls_file(s);
                } else if (S_ISDIR(s->mode)) {
                        print_directory(s);
                }
        }

#endif



        free(s);
        spinfs_deinit();
        return 0;
}
