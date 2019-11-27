#include "spinfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage()
{
        fprintf(stderr, "####### List file or directory #######\n");
        fprintf(stderr, "Format: spinfs_ls path\n");
        fprintf(stderr, "\n");
}

#define TITLE_TEXT_WIDTH        32

/* TODO check logic in passing name to prevent overflow bug with non
 * null-terminated string */
void centerText(char *text, int fieldWidth) {
        int extra = 0;          /* extra pad when fieldWidth is odd */
        if ((strlen(text) % 2) == 1)
                extra = 1;
        int padlen = (fieldWidth - strlen(text)) / 2;
        printf("%*s%s%*s", padlen, "", text, padlen + extra, "");
}

void centerTitleText(char *text, int fieldWidth) {
        printf("\n#####################");
        centerText(text, fieldWidth);
        printf("#####################\n\n");
}

void print_format_dirent(int idx, char *name, uint32_t inum)
{
        char idx_buf[16], name_buf[MAX_NAME_LEN + 1], inum_buf[16];
        sprintf(idx_buf, "%d", idx);
        sprintf(name_buf, "%.*s", MAX_NAME_LEN + 2, name);
        sprintf(inum_buf, "%d", inum);
        centerText(idx_buf, 16);
        printf("|");
        centerText(name_buf, MAX_NAME_LEN + 2);
        printf("|");
        centerText(inum_buf, 16);
        printf("\n");
}

void list_metadata(struct spinfs_raw_inode *s)
{
        printf("Listing %s %.*s, i-node number %d metadata:\n",
                        S_ISDIR(s->mode) ? "directory" : "file",
                        MAX_NAME_LEN, s->name, s->inode_num);
        /* Print details */
        printf("Magic 1             : %*.*s\n", MAX_NAME_LEN, 4, (char *)&s->magic1);
        printf("Name                : %*.*s\n", MAX_NAME_LEN, MAX_NAME_LEN, s->name);
        printf("s-node number       : %*d\n", MAX_NAME_LEN, s->inode_num);
        printf("Mode                : %*s\n", MAX_NAME_LEN,S_ISDIR(s->mode) ? "Directory" : "Regular File");
        printf("UID                 : %*d\n", MAX_NAME_LEN, s->uid);
        printf("GID                 : %*d\n", MAX_NAME_LEN, s->gid);
        printf("Creation time       : %*s", MAX_NAME_LEN, ctime(&(s->ctime)));
        printf("Modification time   : %*s", MAX_NAME_LEN, ctime(&(s->mtime)));
        printf("Flags               : %*s\n", MAX_NAME_LEN, F_ISDEL(s->flags) ? "DELETED" : "0");
        printf("Parent i-node number: %*d\n", MAX_NAME_LEN, s->parent_inode);
        printf("Version             : %*d\n", MAX_NAME_LEN, s->version);
        printf("Data size           : %*d\n", MAX_NAME_LEN, s->data_size);
        printf("Magic 2             : %*.*s\n", MAX_NAME_LEN, 4, (char *)&s->magic2);
}

void list_dir(struct spinfs_raw_inode *s)
{
        list_metadata(s);
        int dirent_count = s->data_size / sizeof(struct dir_entry);
        printf("\nDirectory entry count: %d\n", dirent_count);
        if (dirent_count > 0) {
                printf("------------------------------------------------------------------\n");
                centerText("Entry index", 16);
                printf("|");
                centerText("Name", MAX_NAME_LEN + 2);
                printf("|");
                centerText("I-node number", 16);
                printf("\n");
                printf("------------------------------------------------------------------\n");

                /* loop through directory entry table and print out entries */
                for (int i = 0; i < dirent_count; i++) {
                        print_format_dirent(i,
                                ((struct dir_entry *)s->data)[i].name,
                                ((struct dir_entry *)s->data)[i].inode_num);
                }
        }
}

int list_path(char *path)
{
        centerTitleText("LISTING CONTENT", TITLE_TEXT_WIDTH);

        uint32_t inum = spinfs_check_valid_path(path);
        if (inum == 0) {
                return -1;
        }
        struct spinfs_raw_inode *inode = spinfs_get_inode_from_inum(NULL,
                        inum);
        if (inode == NULL) {
                return -1;
        }
        //print_inode_info(inode, __func__);

        /* Decide if i-node is directory or regular file */
        if (S_ISDIR(inode->mode))
                list_dir(inode);
        else if (S_ISREG(inode->mode))
                list_metadata(inode);

        free(inode);
        centerTitleText("DONE LISTING CONTENT", TITLE_TEXT_WIDTH);
        return 0;
}

int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Too many or too less arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        print_usage();

        char *path = argv[1];
        printf("Path is \"%s\"\n", path);

        spinfs_init(0);
        if (list_path(path) == -1) {
                perror("LS error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        return 0;
}
