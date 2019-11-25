#include "spinfs.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>           // stat
#include <libgen.h>             // basename

void print_usage()
{
        fprintf(stderr, "####### Make new directory on the flash #######\n");
        fprintf(stderr, "Format: spinfs_mkdir [path]\n");
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
        if (argv[1][0] != '/') {
                printf("Path needs to be absolute!\n");
                exit(EXIT_FAILURE);
        }

        spinfs_init();

        //TODO check existence of new file name in target directory, if yes
        //modify the file with the same inode, if not write a new inode with
        //new inode_num
        //TODO get parent_inode based on the path of the destination

        //printf("Argument: %s.\n", argv[1]);
        char *target_basename, *bnamec, *target_dirname, *dnamec;
        bnamec = strdup(argv[1]);
        dnamec = strdup(argv[1]);
        target_basename = basename(bnamec);
        //printf("bnamec: %s.\n", bnamec);
        target_dirname = dirname(dnamec);
        //printf("dnamec: %s.\n", bnamec);
        //printf("Dirname: %s. Basename: %s.\n", target_dirname, target_basename);

        struct spinfs_raw_inode *dirname_inode = malloc(sizeof(*dirname_inode));
        uint32_t dirname_inode_num = 1;  //start traversing from root directory

        /*
         * Parse the path to get the target parent directory i-node
         */
        char *token;
        token = strtok(target_dirname, "/");
        if (token == NULL) {
                // target dirname is root directory
                dirname_inode_num = 1;
        } else {
                while (token != NULL) {
                        dirname_inode = spinfs_read_inode(dirname_inode, spinfs_get_inode_table_entry(dirname_inode_num).physical_addr);
                        dirname_inode_num = find_file_in_dir(dirname_inode, token);
                        if (dirname_inode_num == 0) break;
                        token = strtok(NULL, "/");
                }
        }
        /*
         * Load parent inode structure on flash to dirname_inode
         */
        if (dirname_inode_num == 0) {
                printf("Dirname %s directory not found.\n", token);
                exit(EXIT_FAILURE);
        } else {
                printf("Dirname has inode number: %d\n", dirname_inode_num);
                dirname_inode = spinfs_read_inode(dirname_inode, spinfs_get_inode_table_entry(dirname_inode_num).physical_addr);
        }

        /*
         * Check existing file in parent directory
         */
        int dest_not_exist_flag = 1;
        uint32_t dup_basename_inode_num = find_file_in_dir(dirname_inode, target_basename);
        if (dup_basename_inode_num) {
                struct spinfs_raw_inode *dup_basename_inode = malloc(sizeof(*dup_basename_inode));
                dup_basename_inode = spinfs_read_inode(dup_basename_inode, spinfs_get_inode_table_entry(dup_basename_inode_num).physical_addr);
                if (S_ISDIR(dup_basename_inode->mode)) {
                        printf("Destination directory is already existed.\n");
                        exit(EXIT_FAILURE);
                }
                free(dup_basename_inode);
        }

        printf("Writing new directory.\n");
#if 1
        // dirname_inode is parent directory inode
        struct dir_entry *parent_dir_table = (struct dir_entry *)dirname_inode->data;
        int parent_dir_table_size = dirname_inode->data_size / sizeof(struct dir_entry);

        struct spinfs_raw_inode *new_inode = malloc(sizeof(*new_inode));

        new_inode->magic1 = SPINFS_MAGIC1;
        strncpy(new_inode->name, target_basename, MAX_NAME_LEN);
        new_inode->inode_num = get_inode_table_size() + 1;
        new_inode->uid = getuid();
        new_inode->gid = getgid();
        new_inode->mode = S_IFDIR;
        new_inode->flags = 0;
        new_inode->ctime = time(NULL);
        new_inode->mtime = time(NULL);
        new_inode->parent_inode = dirname_inode->inode_num;
        new_inode->version = 1;
        new_inode->data_size = 0;
        new_inode->magic2 = SPINFS_MAGIC2;

        spinfs_write_inode(new_inode);

        /*
         * Adding new directory entry
         */
        printf("Adding new directory entry in %.*s directory.\n", MAX_NAME_LEN, dirname_inode->name);
        parent_dir_table_size++;
        // Allocate extra memory for 1 more entry
        dirname_inode->data_size += sizeof(struct dir_entry);
        dirname_inode = realloc(dirname_inode, sizeof(*dirname_inode) + dirname_inode->data_size);

        // Assign new value for the new entry
        parent_dir_table = (struct dir_entry *)dirname_inode->data;
        strncpy(parent_dir_table[parent_dir_table_size - 1].name, new_inode->name, MAX_NAME_LEN);
        parent_dir_table[parent_dir_table_size - 1].inode_num = new_inode->inode_num;

        // Update dirname_inode inode and write to flash
        dirname_inode->mtime = time(NULL);
        ++dirname_inode->version;
        printf("\n");
        print_node_info(dirname_inode);
        print_directory(dirname_inode);
        spinfs_write_inode(dirname_inode);








        free(new_inode);
#endif
        free(dirname_inode);
        free(bnamec);
        free(dnamec);
        spinfs_deinit();
        return 0;
}
