#include <stdint.h>
#include <sys/stat.h>
#include <time.h> /* time_t */

#define SPINFS_MAGIC1 0x5350494e   /* SPIN */
#define SPINFS_MAGIC2 0x46537631   /* FSv1 */

#define MAX_NAME_LEN 32

#define DELETED     0x1             /* in flags member */

#ifdef SIMULATED_FLASH
#define SIMULATED_MAIN_FILE_PATH "sim_main_flash"
#define SIMULATED_HEAD_FILE_PATH "sim_sec_reg_1"
#define SIMULATED_TAIL_FILE_PATH "sim_sec_reg_2"
#endif

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
    /*
     * Will be implemented later
     */
    /*
    */
    uint32_t parent_inode;
    uint32_t version;
    uint32_t data_size;
    uint32_t magic2;
    /*
     * Remember padding if needed
     */
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
//extern FILE* sim_main_file;
//extern FILE* sim_head_file;
//extern FILE* sim_tail_file;
#endif

void spinfs_init();
void spinfs_deinit();

/*
 * ******************************************************
 * inode table functions
 * ******************************************************
 */

void spinfs_scan_for_inode_table(); //TODO
void spinfs_update_inode_table(struct spinfs_raw_inode *inode, uint32_t addr);
struct inode_table_entry spinfs_get_inode_table_entry(int inode_num);
uint32_t get_inode_table_size();

/*
 * ******************************************************
 * head, tail functions
 * ******************************************************
 */
void read_head_tail();
void write_head_tail();
void set_head_tail(uint32_t head_new, uint32_t tail_new); //TODO will be removed

// TODO these next 3 might not need for external code
uint32_t get_ht_slot();
uint32_t get_head();
uint32_t get_tail();
void print_head_tail_info();


#ifdef SIMULATED_FLASH
void sim_create_flash_file(char *file);
#endif
void spinfs_erase_sec_reg_1_2();
int32_t spinfs_format();
void spinfs_write_inode(struct spinfs_raw_inode *inode);
void spinfs_update_parent_inode(struct spinfs_raw_inode *inode);
struct spinfs_raw_inode *spinfs_read_inode(struct spinfs_raw_inode *inode, uint32_t addr);










int update_head_tail(uint32_t head_n, uint32_t tail_n);

void print_node_info(struct spinfs_raw_inode *ri);
void print_inode_table(struct inode_table_entry *it);
void ls_file(struct spinfs_raw_inode *inode);
void print_directory(struct spinfs_raw_inode *inode);

uint32_t find_file_in_dir(struct spinfs_raw_inode *inode, char *filename);

void spinfs_scan_fs(uint32_t head, uint32_t tail);
void spinfs_get_inode_at_addr(struct spinfs_raw_inode *s, uint32_t addr);

/*
int32_t spinfs_init(void);
int32_t spinfs_uninit(void);
int32_t spinfs_sync(void);
int32_t spinfs_mount(const char *volume);
int32_t spinfs_unmount(const char *volume);
int32_t spinfs_format(const char *volume);
int32_t spinfs_format();

struct spinfs_raw_inode *createInode(unsigned char *d_buf, uint32_t d_size);
*/

#if 0
struct test_str {
    int i;
    char a;
    int m[];
};
#endif
