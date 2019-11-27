#include "spinfs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>             // basename, dirname

void print_usage()
{
        fprintf(stderr, "####### Create empty file or update i-node mtime on the flash #######\n");
        fprintf(stderr, "Format: spinfs_touch [path]\n");
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
        print_inode_info(parent, __func__);

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

void update_inode_mtime(struct spinfs_raw_inode *s)
{
        print_inode_info(s, __func__);

        s->mtime = time(NULL);
        ++s->version;

        spinfs_write_inode(s);
}

struct spinfs_raw_inode *create_new_empty_file(struct spinfs_raw_inode *new_file, char *name, uint32_t parent_inum)
{
        new_file = realloc(new_file, sizeof(*new_file));
        new_file->magic1 = SPINFS_MAGIC1;
        strncpy(new_file->name, name, MAX_NAME_LEN);
        new_file->inode_num = spinfs_get_next_avail_inum();
        new_file->uid = getuid();
        new_file->gid = getgid();
        new_file->mode = S_IFREG;
        new_file->flags = 0;
        new_file->ctime = time(NULL);
        new_file->mtime = time(NULL);
        new_file->parent_inode = parent_inum;
        new_file->version = 1;
        new_file->data_size = 0;
        new_file->magic2 = SPINFS_MAGIC2;

        spinfs_write_inode(new_file);
        return new_file;
}

int touch_inode(char *bname, char *dname)
{
        centerTitleText("TOUCHING an I-NODE", TITLE_TEXT_WIDTH);

        if (strcmp(bname, "/") == 0) {
                printf("Cannot touch root directory!\n");
                errno = EINVAL;
                return -1;
        }

        uint32_t dir_inum = spinfs_check_valid_path(dname);
        if (dir_inum == 0) return -1;
        struct spinfs_raw_inode *dir_inode = spinfs_get_inode_from_inum(NULL,
                        dir_inum);
        if (dir_inode == NULL) return -1;
        //print_inode_info(dir_inode, __func__);
        uint32_t base_inum = spinfs_is_name_in_dir(dir_inode, bname);
        struct spinfs_raw_inode *base_inode = NULL;
        if (base_inum == 1) {
                return -1;
        } else if (base_inum > 1) {
                printf("Basename exist, update mtime\n");
                base_inode = spinfs_get_inode_from_inum(base_inode, base_inum);
                update_inode_mtime(base_inode);
                update_inode_mtime(dir_inode);
        } else {
                printf("Create new empty file\n");
                base_inode = create_new_empty_file(base_inode, bname,
                                dir_inode->inode_num);
                dir_inode = update_parent_dir(dir_inode, base_inode->name,
                                base_inode->inode_num);
        }

        free(base_inode);
        free(dir_inode);
        centerTitleText("DONE TOUCHING an I-NODE", TITLE_TEXT_WIDTH);
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
        if (touch_inode(target_basename, target_dirname) == -1) {
                perror("TOUCH error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        free(bnamec);
        free(dnamec);
        return 0;
}
