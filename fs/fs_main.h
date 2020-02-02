#ifndef __FS_MAIN__
#define __FS_MAIN__

#define FUSE_USE_VERSION 31
#include "global_types.h"
#include "metadata.h"
#ifdef MONITOR
#include "monitor.h"
//extern struct monitor *global_monitor;
#endif

#define K 1024
#define M 1024 * K
#define G 1024 * M
#define T 1024L * G

#define MAXNAMESIZE 60
#define INODESIZE 256
#define ENTRYPERPAGE PAGESIZE / 4
#define INODEPERPAGE PAGESIZE / INODESIZE
#define DIRPERPAGE PAGESIZE / (MAXNAMESIZE + 4)


#define SUPER_INIT_BN 0
#define D_BITMAP_INIT_BN 1
#define INODE_INIT_BN 78
#define DATA_INIT_BN (INODE_INIT_BN + 100 * K)
#define D_BITMAP_NUM INODE_INIT_BN - D_BITMAP_INIT_BN
#define INODE_BLOCK_NUM DATA_INIT_BN - INODE_INIT_BN
#define TOTAL_INODE_NUM INODE_BLOCK_NUM * (PAGESIZE / INODESIZE)
#define DIRECT_PTR 40
#define INDIRECT_PTR 6
#define D_INDIRECT_PTR 2
#define ROOT_DIR 1

#define INVALID 0
#define VALID 1


typedef struct superblock {
	int fp;
	uint32_t root_directory;
	uint64_t total_block_size;
	uint32_t d_bitmap_init_bn;
	uint32_t inode_init_bn;
	uint32_t list_first;
	uint32_t list_now;
	uint32_t free_inode;
	uint32_t free_d_block;
	uint32_t total_d_blocks;
	uint32_t cur_bit_bn;
	struct d_bitmap *cur_bit;
	char reserve[PAGESIZE - 60];
}superblock;

typedef struct inode {

	struct metadata attr;
	int32_t direct_ptr[DIRECT_PTR];
	int32_t indirect_ptr[INDIRECT_PTR];
	int32_t d_indirect_ptr[D_INDIRECT_PTR];

}inode;

typedef struct i_block {
	struct inode i[PAGESIZE / sizeof(struct inode)];
}i_block;
typedef struct indirect_ptr{
	int32_t ptr[ENTRYPERPAGE];
}indirect_ptr;
typedef struct entry_dir {
	int32_t inode_num;
	char name[MAXNAMESIZE];
}entry_dir;
typedef struct dir_block {
	entry_dir entry[DIRPERPAGE];
}dir_block;
typedef struct d_bitmap {
	uint8_t bitset[PAGESIZE];
}d_bitmap;
typedef struct free_list {
	int32_t free_node[ENTRYPERPAGE - 1];
	int32_t next;
}free_list;

#endif
