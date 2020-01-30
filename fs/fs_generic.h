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

void bitmap_init();
void free_list_init();
#endif
