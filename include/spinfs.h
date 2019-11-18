#include <stdint.h>
#include <sys/stat.h>
#include <time.h> /* time_t */

#define SPIN_FS_MAGIC1 0x5350494e   /* SPIN */
#define SPIN_FS_MAGIC2 0x46537631   /* FSv1 */

#define MAX_NAME_LEN 32

#define WRITE_CTIME 1
#define NOT_WRITE_CTIME 0

struct spinfs_raw_inode {
    uint32_t magic1;
    char name[MAX_NAME_LEN];
    uint32_t inode_num;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t ctime;
    /*
     * Will be implemented later
     */
    /*
    int32_t flags;
    time_t mtime;
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

extern uint32_t head, tail; // might not need to be declared here

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
