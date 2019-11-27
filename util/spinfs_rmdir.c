#include "spinfs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage()
{
        fprintf(stderr, "####### Remove an empty directory #######\n");
        fprintf(stderr, "Format: spinfs_rmdir path\n");
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

/*
 * Remove the deleted i-node from parent directory entry table
 */
struct spinfs_raw_inode *update_parent_dir(struct spinfs_raw_inode *parent, uint32_t deleted_inum)
{
        print_inode_info(parent, __func__);

        /* Save old dirent table to buffer */
        struct dir_entry *old_table = malloc(parent->data_size);
        memcpy(old_table, parent->data, parent->data_size);

        /* Find index of the deleted entry in the table */
        int dirent_count = parent->data_size / sizeof(struct dir_entry);
        int deleted_index = spinfs_get_dirent_index(old_table, dirent_count,
                        deleted_inum);
        if (deleted_index == -1) {
                printf("Fatal: Cannot update parent directory\n");
                exit(EXIT_FAILURE);
        }

        /* Reduce the size of  parent directory by one dirent */
        dirent_count--;
        parent->data_size -= sizeof(struct dir_entry);
        parent = realloc(parent, sizeof(*parent) + parent->data_size);
        struct dir_entry *new_table = (struct dir_entry *)parent->data;
        /* Copy new table to parent i-node */
        memcpy(new_table, old_table, deleted_index * sizeof(struct dir_entry));
        memcpy(&new_table[deleted_index], &old_table[deleted_index + 1],
                (dirent_count - deleted_index) * sizeof(struct dir_entry));

        /* Update parent i-node metadata */
        parent->mtime = time(NULL);
        ++parent->version;

        spinfs_write_inode(parent);
        free(old_table);
        return parent;
}

void update_deleted_dir(struct spinfs_raw_inode *s)
{
        s->flags |= DELETED;
        s->mtime = time(NULL);
        ++s->version;

        spinfs_write_inode(s);
}

int remove_dir(char *path)
{
        centerTitleText("REMOVING a DIRECTORY", TITLE_TEXT_WIDTH);

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
        if (!S_ISDIR(inode->mode)) {
                errno = ENOTDIR;
                return -1;
        }
        /* Check if directory is empty */
        if (inode->data_size) {
                errno = ENOTEMPTY;
                return -1;
        }
        update_deleted_dir(inode);

        struct spinfs_raw_inode *parent = spinfs_get_inode_from_inum(NULL,
                        inode->parent_inode);
        parent = update_parent_dir(parent, inum);

        free(inode);
        free(parent);
        centerTitleText("DONE REMOVING a DIRECTORY", TITLE_TEXT_WIDTH);
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

        spinfs_init();
        if (remove_dir(path) == -1) {
                perror("RMDIR error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        return 0;
}
