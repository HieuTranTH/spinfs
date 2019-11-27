#include <stdint.h>
#include <sys/stat.h>   /* mode_t */
#include <time.h> /* time_t */

#define SPINFS_MAGIC1 0x5350494e   /* SPIN */
#define SPINFS_MAGIC2 0x46537631   /* FSv1 */

#define MAX_NAME_LEN 32

#define DELETED     0x1             /* in flags member */
#define F_ISDEL(flags)  ((flags & 0x1) == DELETED)

#ifdef SIMULATED_FLASH
#define SIMULATED_MAIN_FILE_PATH "sim_main_flash"
#define SIMULATED_HEAD_FILE_PATH "sim_sec_reg_1"
#define SIMULATED_TAIL_FILE_PATH "sim_sec_reg_2"
#endif

/*
 * Structure declarations
 */
struct spinfs_raw_inode {
    uint32_t magic1;
    char name[MAX_NAME_LEN];
    uint32_t inode_num;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t ctime;
    time_t mtime;
    uint32_t flags;
    uint32_t parent_inode;
    uint32_t version;
    uint32_t data_size;
    uint32_t magic2;
    char data[];
};

struct dir_entry {
    char name[MAX_NAME_LEN];
    uint32_t inode_num;
};

struct inode_table_entry {
    uint32_t physical_addr;
    uint32_t version;
};

#ifdef SIMULATED_FLASH
void spinfs_create_sim_flash(char *file);
#endif

/*
 * Initializing and De-initializing spinfs functions
 * to load head, tail
 * to allocate inode_table dynamically
 * to open simulated flash files
 */
void spinfs_init();
void spinfs_deinit();

/*
 * ******************************************************
 * head, tail functions
 * ******************************************************
 */
void spinfs_read_ht_slot();
void spinfs_read_head_tail();
void spinfs_write_head_tail();

/*
 * ******************************************************
 * inode table functions
 * ******************************************************
 */
void spinfs_scan_fs();
void spinfs_update_itable(struct spinfs_raw_inode *i, uint32_t addr);
void spinfs_erase_itable();
uint32_t spinfs_get_next_avail_inum();

/*
 * ******************************************************
 * inode functions
 * ******************************************************
 */
struct spinfs_raw_inode *spinfs_read_inode(struct spinfs_raw_inode *i,
        uint32_t addr);
struct spinfs_raw_inode *spinfs_get_inode_from_inum(struct spinfs_raw_inode *i,
        uint32_t inum);
int spinfs_check_valid_inode(struct spinfs_raw_inode *i);
void spinfs_write_inode(struct spinfs_raw_inode *i);
uint32_t spinfs_is_name_in_dir(struct spinfs_raw_inode *s, char *name);

/*
 * ******************************************************
 * dirent functions
 * ******************************************************
 */
int spinfs_get_dirent_index(struct dir_entry *t, int size, uint32_t inum);

/*
 * ******************************************************
 * spinfs functions
 * ******************************************************
 */
void spinfs_erase_fs();
void spinfs_erase_sec_reg_1_2();
void spinfs_format();
uint32_t spinfs_check_valid_path(char *path);

/*
 * ******************************************************
 * Debug functions
 * ******************************************************
 */
void print_head_tail_info(const char *caller);
void print_itable_info(const char *caller);
void print_inode_info(struct spinfs_raw_inode *i, const char *caller);
