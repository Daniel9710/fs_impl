#ifndef __FS_FILE__
#define __FS_FILE__

#include "fs_main.h"
#include <fuse3/fuse.h>
#include <fcntl.h>

int fs_open (const char *, struct fuse_file_info *);
int fs_create (const char *, mode_t, struct fuse_file_info *);
int fs_read (const char *, char *, size_t, off_t, struct fuse_file_info *);
int fs_write (const char *, const char *, size_t, off_t, struct fuse_file_info *);
int fs_flush (const char *, struct fuse_file_info *);
int fs_unlink (const char *);
int fs_truncate (const char *, off_t, struct fuse_file_info *);
int fs_release (const char *, struct fuse_file_info *);
int fs_fsync (const char *, int isdatasync, struct fuse_file_info *);

#endif
