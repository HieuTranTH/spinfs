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

#define TITLE_TEXT_WIDTH        32

/* TODO layout is not consistent if fieldWidth is odd */
void centerText(char *text, int fieldWidth) {
        int padlen = (fieldWidth - strlen(text)) / 2;
        printf("%*s%s%*s", padlen, "", text, padlen, "");
}

void centerTitleText(char *text, int fieldWidth) {
        printf("\n#####################");
        centerText(text, fieldWidth);
        printf("#####################\n\n");
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
        centerTitleText("RUNNING SPINFS FORMATTER", TITLE_TEXT_WIDTH);
        spinfs_format();
        write_root_inode();

        centerTitleText("FINISHED SPINFS FORMATTER", TITLE_TEXT_WIDTH);
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
