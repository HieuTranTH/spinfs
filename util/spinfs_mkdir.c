#include "spinfs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>             // basename, dirname

void print_usage()
{
        fprintf(stderr, "####### Make new directory on the flash #######\n");
        fprintf(stderr, "Format: spinfs_mkdir [path]\n");
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

struct spinfs_raw_inode *update_parent_dir(struct spinfs_raw_inode *parent, char *new_name, uint32_t new_inum)
{
        /* Add new dirent to parent directory */
        parent->data_size += sizeof(struct dir_entry);
        parent = realloc(parent, sizeof(*parent) + parent->data_size);
        int dirent_count = parent->data_size / sizeof(struct dir_entry);
        /* Assign new value for the new entry */
        struct dir_entry *dir_table = (struct dir_entry *)parent->data;
        strncpy(dir_table[dirent_count - 1].name, new_name, MAX_NAME_LEN);
        dir_table[dirent_count - 1].inode_num = new_inum;

        /* Update parent i-node metadata */
        parent->mtime = time(NULL);
        ++parent->version;

        spinfs_write_inode(parent);
        return parent;
}

struct spinfs_raw_inode *create_new_empty_dir(struct spinfs_raw_inode *new_dir, char *name, uint32_t parent_inum)
{
        new_dir = realloc(new_dir, sizeof(*new_dir));
        new_dir->magic1 = SPINFS_MAGIC1;
        strncpy(new_dir->name, name, MAX_NAME_LEN);
        new_dir->inode_num = spinfs_get_next_avail_inum();
        new_dir->uid = getuid();
        new_dir->gid = getgid();
        new_dir->mode = S_IFDIR;
        new_dir->flags = 0;
        new_dir->ctime = time(NULL);
        new_dir->mtime = time(NULL);
        new_dir->parent_inode = parent_inum;
        new_dir->version = 1;
        new_dir->data_size = 0;
        new_dir->magic2 = SPINFS_MAGIC2;

        spinfs_write_inode(new_dir);
        return new_dir;
}

int make_directory(char *bname, char *dname)
{
        centerTitleText("MAKING a NEW DIRECTORY", TITLE_TEXT_WIDTH);

        if (strcmp(bname, "/") == 0) {
                printf("Cannot make root directory!\n");
                errno = EINVAL;
                return -1;
        }

        uint32_t dir_inum = spinfs_check_valid_path(dname);
        if (dir_inum == 0) return -1;
        struct spinfs_raw_inode *dir_inode = spinfs_get_inode_from_inum(NULL,
                        dir_inum);
        if (dir_inode == NULL) return -1;
        //print_inode_info(dir_inode, __func__);
        if (spinfs_is_name_in_dir(dir_inode, bname)) return -1;
        struct spinfs_raw_inode *base_inode = create_new_empty_dir(NULL,
                        bname, dir_inode->inode_num);
        dir_inode = update_parent_dir(dir_inode, base_inode->name, base_inode->inode_num);

        free(base_inode);
        free(dir_inode);
        centerTitleText("DONE MAKING a NEW DIRECTORY", TITLE_TEXT_WIDTH);
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
        printf("Path is \"%s\"\n", argv[1]);

        /* Parse dirname and basename from command line argument */
        char *target_basename, *bnamec, *target_dirname, *dnamec;
        bnamec = strdup(argv[1]);
        dnamec = strdup(argv[1]);
        target_basename = basename(bnamec);
        target_dirname = dirname(dnamec);

        spinfs_init(0);
        if (make_directory(target_basename, target_dirname) == -1) {
                perror("MKDIR error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        free(bnamec);
        free(dnamec);
        return 0;
}
