#include "spinfs.h"
#include "spi_flash.h"
#include <errno.h>
#include <libgen.h>     /* basename */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

void print_usage()
{
        fprintf(stderr, "####### Copy to and from the flash#######\n");
        fprintf(stderr, "Format: spinfs_cp src dest\n");
        fprintf(stderr, "src is not dir and dest has to be a dir\n");
        fprintf(stderr, "Append :spinfs: to only one of the path to specify upload or download direction for the flash\n");
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

int download_from_flash(char *src, char *dest)
{
        centerTitleText("COPYING FILE from FLASH", TITLE_TEXT_WIDTH);

        /* Validate src path and get src i-node struct */
        uint32_t src_inum = spinfs_check_valid_path(src);
        if (src_inum == 0) return -1;
        struct spinfs_raw_inode *src_inode = spinfs_get_inode_from_inum(NULL,
                        src_inum);
        if (src_inode == NULL) return -1;
        if (S_ISDIR(src_inode->mode)) {
                errno = EISDIR;
                return -1;
        }
        print_inode_info(src_inode, __func__);

        /* Validate dest path and open dest file to write */
        struct stat dest_st;
        if (stat(dest, &dest_st) == -1) {
                perror("Stat file error");
                exit(EXIT_FAILURE);
        }
        if (!S_ISDIR(dest_st.st_mode)) {
                errno = ENOTDIR;
                return -1;
        }
        /*
         * Form dest file name from dest dir name and src file name
         * 1 extra byte for '/' character and another for null character
         */
        char dest_name[strlen(dest) + 1 + MAX_NAME_LEN + 1];
        sprintf(dest_name, "%s/%.*s", dest, MAX_NAME_LEN, src_inode->name);
        printf("dest file name: %s\n", dest_name);
        FILE *dest_fp = fopen(dest_name, "w");
        if (dest_fp == NULL) {
                return -1;
        }

        fwrite(src_inode->data, 1, src_inode->data_size, dest_fp);

        fclose(dest_fp);
        free(src_inode);
        centerTitleText("DONE COPYING FILE from FLASH", TITLE_TEXT_WIDTH);
        return 0;
}

struct spinfs_raw_inode *update_old_file(struct spinfs_raw_inode *s, off_t size, FILE *fp)
{
        s = realloc(s, sizeof(*s) + size);
        s->mtime = time(NULL);
        ++s->version;
        s->data_size = size;
        if (size > 0) {
                fseek(fp, 0, SEEK_SET);
                fread(s->data, 1, size, fp);
        }

        spinfs_write_inode(s);
        return s;
}

void update_inode_mtime(struct spinfs_raw_inode *s)
{
        print_inode_info(s, __func__);

        s->mtime = time(NULL);
        ++s->version;

        spinfs_write_inode(s);
}

struct spinfs_raw_inode *create_new_file(struct spinfs_raw_inode *new_file, char *name, off_t size, uint32_t parent_inum, FILE *fp)
{
        new_file = realloc(new_file, sizeof(*new_file) + size);
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
        new_file->data_size = size;
        new_file->magic2 = SPINFS_MAGIC2;
        if (size > 0) {
                fseek(fp, 0, SEEK_SET);
                fread(new_file->data, 1, size, fp);
        }

        spinfs_write_inode(new_file);
        return new_file;
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

int upload_to_flash(char *src, char *dest)
{
        centerTitleText("COPYING FILE to FLASH", TITLE_TEXT_WIDTH);

        /* Validate src path and open FILE handle to src file */
        struct stat src_st;
        if (stat(src, &src_st) == -1) {
                perror("Stat file error");
                exit(EXIT_FAILURE);
        }
        if (S_ISDIR(src_st.st_mode)) {
                errno = EISDIR;
                return -1;
        }
        if (src_st.st_size > MAIN_FLASH_SIZE / 2) {
                errno = EFBIG;
                return -1;
        }
        FILE *src_fp = fopen(src, "r");
        if (src_fp == NULL) {
                return -1;
        }

        /* Validate dest path and get dest dir i-node struct */
        uint32_t dir_inum = spinfs_check_valid_path(dest);
        if (dir_inum == 0) return -1;
        struct spinfs_raw_inode *dir_inode = spinfs_get_inode_from_inum(NULL,
                        dir_inum);
        if (dir_inode == NULL) return -1;
        //print_inode_info(dir_inode, __func__);
        uint32_t base_inum = spinfs_is_name_in_dir(dir_inode, basename(src));
        struct spinfs_raw_inode *base_inode = NULL;
        if (base_inum == 1) {           /* dir_inode is not a directory */
                return -1;
        } else if (base_inum > 1) {
                base_inode = spinfs_get_inode_from_inum(base_inode, base_inum);
                if (S_ISDIR(base_inode->mode)) {
                        errno = EISDIR;
                        return -1;
                }
                printf("Target filename exist, rewrite\n");
                base_inode = update_old_file(base_inode, src_st.st_size, src_fp);
                update_inode_mtime(dir_inode);
        } else {
                printf("Create new file\n");
                base_inode = create_new_file(base_inode, basename(src),
                                src_st.st_size, dir_inode->inode_num, src_fp);
                dir_inode = update_parent_dir(dir_inode, base_inode->name,
                                base_inode->inode_num);
        }

        free(base_inode);
        free(dir_inode);
        centerTitleText("DONE COPYING FILE to FLASH", TITLE_TEXT_WIDTH);
        return 0;
}

int copy_file(char *src, char *dest)
{
        char prefix[] = ":spinfs:";

        if ((strncmp(src, prefix, strlen(prefix)) == 0)
                && (strncmp(dest, prefix, strlen(prefix)) != 0)) {
                src = src + strlen(prefix);
                if (download_from_flash(src, dest) == -1)
                        return -1;
        } else if ((strncmp(src, prefix, strlen(prefix)) != 0)
                && (strncmp(dest, prefix, strlen(prefix)) == 0)) {
                dest = dest + strlen(prefix);
                if (upload_to_flash(src, dest) == -1)
                        return -1;
        } else {
                errno = EINVAL;
                return -1;
        }
        return 0;
}

int main(int argc, char *argv[])
{
        if (argc != 3) {
                printf("Too many or too less arguments\n\n");
                print_usage();
                exit(EXIT_FAILURE);
        }
        print_usage();
        char *src = argv[1];
        char *dest = argv[2];
        printf("Source file          : \"%s\"\n", src);
        printf("Destination directory: \"%s\"\n", dest);

        spinfs_init(0);
        if (copy_file(src, dest) == -1) {
                perror("CP error");
                exit(EXIT_FAILURE);
        }
        spinfs_deinit();

        return 0;
}
