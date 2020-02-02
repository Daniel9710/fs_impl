#ifndef __FS_GENERIC__
#define __FS_GECERIC__

#include "fs_main.h"
#include <fuse3/fuse.h>
#include <fcntl.h>

int fs_getattr (const char *, struct stat *, struct fuse_file_info *);
int fs_utimens (const char *, const struct timespec ts[2], struct fuse_file_info *);
int fs_chmod (const char *, mode_t, struct fuse_file_info *);
int fs_chown (const char *, uid_t, gid_t, struct fuse_file_info *);
int fs_rename (const char *, const char *, unsigned int);
int fs_access (const char *, int);
int fs_symlink (const char *, const char *);
int fs_readlink (const char *, char *, size_t);
void *fs_init (struct fuse_conn_info *conn, struct fuse_config *cfg);
void fs_destroy (void *private_data);

void super_init();
void bitmap_init();
void free_list_init();
void super_write();
void super_read();
void bitmap_read(d_bitmap *bitmap, uint32_t block_num);
void bitmap_write(d_bitmap *bitmap, uint32_t block_num);
void bitmap_update(uint32_t data_block_num, uint32_t type);
void data_read(void *data, uint32_t block_num);
void data_write(void *data, uint32_t block_num);
void inode_read(inode *node, int32_t inode_block_num);
void inode_write(inode *node, uint32_t inode_block_num);
int search_bitmap(int *arr, int num);
int new_inode();
void free_inode(int block_num);
int inode_trace(const char *path, inode *node, char *file);
void metadata_init(struct metadata *meta, mode_t mode, int link, size_t size, uint64_t ino);
void cur_bit_test();
#endif
