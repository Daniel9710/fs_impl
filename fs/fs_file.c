#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include "fs_file.h"
#include "metadata.h"
#include "fs_main.h"

extern struct monitor *global_monitor;
extern struct superblock spb;

int fs_open (const char *path, struct fuse_file_info *fi) {
	fi->keep_cache = 1;

	return 0;
}

int fs_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
	fi->keep_cache = 1;

	return 0;
}

int fs_read (const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	
	return 0;
}

int fs_write (const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	
	return 0;
}

int fs_flush (const char *path, struct fuse_file_info *fi) {

	return 0;
}

int fs_unlink (const char *path) {
	
	return 0;
}

int fs_truncate (const char *path, off_t off, struct fuse_file_info *fi) {
	
	return 0;
}

int fs_release (const char *path, struct fuse_file_info *fi) {

	return 0;
}

int fs_fsync (const char *path, int isdatasync, struct fuse_file_info *fi) {
	
	return 0;
}
