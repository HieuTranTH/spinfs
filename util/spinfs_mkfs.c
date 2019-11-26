#include "spinfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage()
{
        fprintf(stderr, "####### Format the flash #######\n");
        fprintf(stderr, "Format: spinfs_mkfs\n");
        fprintf(stderr, "\n");
}

int write_root_inode()
{
        /*
         * Write root directory inode
         */
        struct spinfs_raw_inode *root_inode = malloc(sizeof(*root_inode));

        root_inode->magic1 = SPINFS_MAGIC1;
        strncpy(root_inode->name, "/", MAX_NAME_LEN);
        root_inode->inode_num = 1;
        root_inode->uid = getuid();
        root_inode->gid = getgid();
        root_inode->mode = S_IFDIR;
        root_inode->flags = 0;
        root_inode->ctime = time(NULL);
        root_inode->mtime = time(NULL);
        root_inode->parent_inode = 0;
        root_inode->version = 1;
        root_inode->data_size = 0;
        root_inode->magic2 = SPINFS_MAGIC2;

        spinfs_write_inode(root_inode);
        free(root_inode);

        return 0;
}

int format()
{
        printf("\n##################### %30s #####################\n\n", "RUNNING SPINFS FORMATTER");
        spinfs_format();
        write_root_inode();

        printf("\n##################### %30s #####################\n\n", "FINISHED SPINFS FORMATTER");
        return 0;
}

int main(int argc, char *argv[])
{
        print_usage();

        spinfs_init();
        if (format() == -1) {
                perror("MKFS error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        return 0;
}
