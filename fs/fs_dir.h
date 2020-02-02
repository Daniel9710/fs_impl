#ifndef __FS_DIR__
#define __FS_DIR__

#include "fs_main.h"
#include <fuse3/fuse.h>
#include <fcntl.h>

int fs_opendir (const char *, struct fuse_file_info *);
int fs_mkdir (const char *, mode_t);
int fs_readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags);
int fs_rmdir (const char *);
int fs_releasedir (const char *, struct fuse_file_info *);
int fs_fsyncdir (const char *, int, struct fuse_file_info *);

int search_dir(inode *node, const char *ptr);
int update_dir(inode *node, int inum, const char *ptr, int type);
void update_direntry(dir_block *dir, int inum, const char *ptr, int idx, int blk, int type);
int delete_dir(inode *node, int inum);
int is_dir_empty(inode *node);
#endif
