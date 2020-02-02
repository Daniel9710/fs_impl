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
#include "fs_generic.h"
#include "fs_dir.h"

extern struct monitor *global_monitor;
extern struct superblock spb;

int fs_open (const char *path, struct fuse_file_info *fi) {
	fi->keep_cache = 1;
	inode node;
	char ppath[56];
	int cwd;
	struct metadata mm;
	struct fuse_context *fs_cxt = fuse_get_context();

	if((cwd = inode_trace(path, &node, ppath)) == -1)
		return -EINVAL;
	fi->fh = search_dir(&node, ppath);
	mm = node.attr;
	if (fi->flags & O_RDONLY || fi->flags & O_RDWR) {
		if(!(((mm.mode & S_IRUSR) && (mm.uid == fs_cxt->uid)) || ((mm.mode & S_IRGRP) && (mm.gid == fs_cxt->gid)) || (mm.mode & S_IROTH)))
			return -1;
	}
	if (fi->flags & O_WRONLY || fi->flags & O_RDWR) {
		if(!(((mm.mode & S_IWUSR) && (mm.uid == fs_cxt->uid)) || ((mm.mode & S_IWGRP) && (mm.gid == fs_cxt->gid)) || (mm.mode & S_IWOTH)))
			return -1;
	}

	return 0;
}

int fs_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
	fi->keep_cache = 1;
	inode node, dir_node;
	char ppath[56];
	int inum, cwd;
	
	/*
	if((cwd = inode_trace(path, &dir_node, ppath)) == -1)
		cwd = spb.root_directory;
	else {
		cwd = search_dir(&dir_node, ppath);
		if(cwd == -1) {
			if(fi->flags & O_CREAT)
				return -ENOENT;
			inum = new_inode();
			memset(&node, -1, struct(inode));
			update_dir(&dir_node, inum, ppath,S_IFFIL);
			dir_node.attr.nlink++;
			dir_node.attr.mtime = dir_node.attr.ctime = time(NULL);
			inode_write(&dir_node, cwd);

		}
		if(cwd != -1 && (fi->flags & O_EXCL) && (fi->flags & O_CREAT))
			return -EEXIST;
	}
	inode_read(&node, cwd);
	*/
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
void remove_file(inode *node) {
	int i,j,k,l;
	indirect_ptr in_ptr, d_in_ptr;
	for(i = 0; i < DIRECT_PTR; i++) {
		if(node->direct_ptr[i] != -1)
			bitmap_update(node->direct_ptr[i], INVALID);
		else
			return;
	}
	for(k = 0; k < INDIRECT_PTR; k++) {
		if(node->indirect_ptr[k] != -1) {
			data_read((void *)&in_ptr, node->indirect_ptr[k]);
			for(i = 0; i < ENTRYPERPAGE; i++) {
				if(in_ptr.ptr[i] != -1)
					bitmap_update(in_ptr.ptr[i], INVALID);
				else
					return;
			}
		}
		else
			return;
	}
	for(l = 0; l < D_INDIRECT_PTR; l++) {
		if(node->d_indirect_ptr[l] != -1) {
			data_read((void *)&d_in_ptr, node->d_indirect_ptr[k]);
			for(k = 0; k < INDIRECT_PTR; k++) {
				if(d_in_ptr.ptr[k] != -1) {
					data_read((void *)&in_ptr, d_in_ptr.ptr[k]);
					for(i = 0; i < ENTRYPERPAGE; i++) {
						if(in_ptr.ptr[i] != -1)
							bitmap_update(in_ptr.ptr[i], INVALID);
						else
							return;
					}
				}
				else
					return;
			}
		}
		else
			return;
	}
}
