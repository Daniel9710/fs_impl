#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include "fs_file.h"
#include "metadata.h"
#include "fs_main.h"
#include "fs_generic.h"
#include "fs_dir.h"

extern struct monitor *global_monitor;
extern struct superblock spb;

int fs_open (const char *path, struct fuse_file_info *fi) {
	fi->keep_cache = 1;
	inode *node = (inode *)malloc(sizeof(inode));
	char ppath[56];
	int cwd;
	struct metadata mm;
	struct fuse_context *fs_cxt = fuse_get_context();

	if((cwd = inode_trace(path, node, ppath)) == -1)
		return -EINVAL;
	cwd = search_dir(node, ppath);
	inode_read(node,cwd);
	mm = node->attr;
	if (fi->flags & O_RDONLY || fi->flags & O_RDWR) {
		if(!(((mm.mode & S_IRUSR) && (mm.uid == fs_cxt->uid)) || ((mm.mode & S_IRGRP) && (mm.gid == fs_cxt->gid)) || (mm.mode & S_IROTH)))
			return -1;
	}
	if (fi->flags & O_WRONLY || fi->flags & O_RDWR) {
		if(!(((mm.mode & S_IWUSR) && (mm.uid == fs_cxt->uid)) || ((mm.mode & S_IWGRP) && (mm.gid == fs_cxt->gid)) || (mm.mode & S_IWOTH)))
			return -1;
	}
	fi->fh = node;

	return 0;
}

int fs_create (const char *path, mode_t mode, struct fuse_file_info *fi) {
	fi->keep_cache = 1;
	inode *node = (inode *)malloc(sizeof(inode));

	memset(node, -1, sizeof(inode));
	metadata_init(&node->attr, mode, 1, 0, -1);

	fi->fh = node;

	return 0;
}

int fs_read (const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	off_t blknum;
	uint64_t end_cur, end_blk;
	size_t cnt = 0;
	int i, j, k;
	indirect_ptr in_ptr, d_in_ptr;
	inode *node = (inode *)fi->fh;
	char r_buf[PAGESIZE];

	for(blknum = off / PAGESIZE; blknum < DIRECT_PTR; blknum++) {
		if(node->direct_ptr[blknum] != -1) {
			data_read((void *)&r_buf, node->direct_ptr[blknum]);
			end_cur = (size - cnt)>PAGESIZE?PAGESIZE:(size - cnt);
			for(i = (off + cnt) % PAGESIZE; r_buf[i] != '\0' && i < end_cur;)
				buf[cnt++] = r_buf[i++];
			if(r_buf[i] == '\0' || size == cnt)
				return cnt;
		}
		else
			return cnt;
	}
	for(j = (((cnt + off) / PAGESIZE) - DIRECT_PTR) / ENTRYPERPAGE; j < INDIRECT_PTR; j++) {
		if(node->indirect_ptr[j] != -1) {
			data_read((void *)&in_ptr, node->indirect_ptr[j]);
			for(blknum = (((cnt + off) / PAGESIZE) - DIRECT_PTR) % ENTRYPERPAGE; blknum < ENTRYPERPAGE; blknum++) {
				if(in_ptr.ptr[blknum] != -1) {
					data_read((void *)&r_buf, in_ptr.ptr[blknum]);
					end_cur = (size - cnt)>PAGESIZE?PAGESIZE:(size - cnt);
					for(i = (off + cnt) % PAGESIZE; r_buf[i] != '\0' && i < end_cur;)
						buf[cnt++] = r_buf[i++];
					if(r_buf[i] == '\0' || size == cnt)
						return cnt;
				}
				else
					return cnt;
			}
		}
		else return cnt;
	}
	for(k = (((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) / (ENTRYPERPAGE * ENTRYPERPAGE); k < D_INDIRECT_PTR; k++) {
		if(node->d_indirect_ptr[k] != -1) {
			data_read((void *)&d_in_ptr, node->d_indirect_ptr[k]);
			for(j = ((((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) % (ENTRYPERPAGE * ENTRYPERPAGE)) / ENTRYPERPAGE; j < INDIRECT_PTR; j++) {
				if(d_in_ptr.ptr[j] != -1) {
					data_read((void *)&in_ptr, d_in_ptr.ptr[j]);
					for(blknum = ((((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) % (ENTRYPERPAGE * ENTRYPERPAGE)) % ENTRYPERPAGE; blknum < ENTRYPERPAGE; blknum++) {
						if(in_ptr.ptr[blknum] != -1) {
							data_read((void *)&r_buf, in_ptr.ptr[blknum]);
							end_cur = (size - cnt)>PAGESIZE?PAGESIZE:(size - cnt);
							for(i = (off + cnt) % PAGESIZE; r_buf[i] != '\0' && i < end_cur;)
								buf[cnt++] = r_buf[i++];
							if(r_buf[i] == '\0' || size == cnt)
								return cnt;
						}
						else
							return cnt;
					}
				}
				else
					return cnt;
			}
		}
		else
			return cnt;
	}
	return 0;
}

int fs_write (const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	off_t blknum;
	uint64_t end_cur;
	size_t cnt = 0;
	int i, j, k, arr[3];
	indirect_ptr in_ptr, d_in_ptr;
	inode *node = (inode *)fi->fh;
	char w_buf[PAGESIZE];

	for(blknum = off / PAGESIZE; blknum < DIRECT_PTR; blknum++) {
		if(node->direct_ptr[blknum] == -1) {
			search_bitmap(arr, 1);
			node->direct_ptr[blknum] = arr[0];
			memset(w_buf, 0, PAGESIZE);
		}
		else
			data_read((void *)&w_buf, node->direct_ptr[blknum]);
		end_cur = (size - cnt)>PAGESIZE?PAGESIZE:(size - cnt);
		for(i = (off + cnt) % PAGESIZE; w_buf[i] != '\0' && i < end_cur;)
			w_buf[i++] = buf[cnt++];
		data_write((void *)&w_buf, node->direct_ptr[blknum]);
		if(w_buf[i] == '\0' || size == cnt) {
			node->attr.size += cnt;
			return cnt;
		}
	}
	for(j = (((cnt + off) / PAGESIZE) - DIRECT_PTR) / ENTRYPERPAGE; j < INDIRECT_PTR; j++) {
		blknum = (((cnt + off) / PAGESIZE) - DIRECT_PTR) % ENTRYPERPAGE;
		if(node->indirect_ptr[j] != -1) {
			search_bitmap(arr,2);
			node->indirect_ptr[j] = arr[0];
			memset(&in_ptr, -1, PAGESIZE);
			in_ptr.ptr[blknum] = arr[1];
			memset(w_buf, 0, PAGESIZE);
		}
		else
			data_read((void *)&in_ptr, node->indirect_ptr[j]);

		for(; blknum < ENTRYPERPAGE; blknum++) {
			if(in_ptr.ptr[blknum] == -1) {
				search_bitmap(arr, 1);
				in_ptr.ptr[blknum] = arr[0];
				memset(w_buf, 0, PAGESIZE);
			}
			else
				data_read((void *)&w_buf, in_ptr.ptr[blknum]);
			end_cur = (size - cnt)>PAGESIZE?PAGESIZE:(size - cnt);
			for(i = (off + cnt) % PAGESIZE; w_buf[i] != '\0' && i < end_cur;)
				w_buf[i++] = buf[cnt++];
			data_write((void *)&w_buf, in_ptr.ptr[blknum]);
			if(w_buf[i] == '\0' || size == cnt) {
				data_write((void *)&in_ptr, node->indirect_ptr[j]);
				node->attr.size += cnt;
				return cnt;
			}
		}
		data_write((void *)&in_ptr, node->indirect_ptr[j]);
	}
	for(k = (((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) / (ENTRYPERPAGE * ENTRYPERPAGE); k < D_INDIRECT_PTR; k++) {
		j = ((((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) % (ENTRYPERPAGE * ENTRYPERPAGE)) / ENTRYPERPAGE;
		if(node->d_indirect_ptr[k] == -1) {
			blknum = ((((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) % (ENTRYPERPAGE * ENTRYPERPAGE)) % ENTRYPERPAGE;
			search_bitmap(arr,3);
			node->d_indirect_ptr[k] = arr[0];
			memset(&d_in_ptr, -1, PAGESIZE);
			d_in_ptr.ptr[j] = arr[1];
			memset(&in_ptr, -1, PAGESIZE);
			in_ptr.ptr[blknum] = arr[2];
			memset(w_buf, 0, PAGESIZE);
		}
		else
			data_read((void *)&d_in_ptr, node->d_indirect_ptr[k]);

		for(; j < INDIRECT_PTR; j++) {
			if(d_in_ptr.ptr[j] != -1) {
				search_bitmap(arr,2);
				d_in_ptr.ptr[j] = arr[0];
				memset(&in_ptr, -1, PAGESIZE);
				in_ptr.ptr[blknum] = arr[1];
				memset(w_buf, 0, PAGESIZE);
			}
			else
				data_read((void *)&in_ptr, d_in_ptr.ptr[j]);

			for(blknum = ((((cnt + off) / PAGESIZE) - (DIRECT_PTR + ENTRYPERPAGE * INDIRECT_PTR)) % (ENTRYPERPAGE * ENTRYPERPAGE)) % ENTRYPERPAGE; blknum < ENTRYPERPAGE; blknum++) {
				if(in_ptr.ptr[blknum] != -1) {
					search_bitmap(arr, 1);
					in_ptr.ptr[blknum] = arr[0];
					memset(w_buf, 0, PAGESIZE);
				}
				else
					data_read((void *)&w_buf, in_ptr.ptr[blknum]);

				end_cur = (size - cnt)>PAGESIZE?PAGESIZE:(size - cnt);
				for(i = (off + cnt) % PAGESIZE; w_buf[i] != '\0' && i < end_cur;)
					w_buf[i++] = buf[cnt++];
				data_write((void *)&w_buf, in_ptr.ptr[blk_num]);
				if(w_buf[i] == '\0' || size == cnt) {
					data_write((void *)&d_in_ptr, node->d_indirect_ptr[k]);
					data_write((void *)&in_ptr, d_in_ptr.ptr[j]);
					node->attr.size += cnt;
					return cnt;
				}
			}
			data_write((void *)&in_ptr, d_in_ptr.ptr[j]);
		}
		data_write((void *)&d_in_ptr, node->d_indirect_ptr[k]);
	}
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
	int i, k, l;
	indirect_ptr in_ptr, d_in_ptr;
	for(i = 0; i < DIRECT_PTR; i++) {
		if(node->direct_ptr[i] != -1)
			bitmap_update(node->direct_ptr[i], INVALID);
	}
	for(k = 0; k < INDIRECT_PTR; k++) {
		if(node->indirect_ptr[k] != -1) {
			data_read((void *)&in_ptr, node->indirect_ptr[k]);
			for(i = 0; i < ENTRYPERPAGE; i++) {
				if(in_ptr.ptr[i] != -1)
					bitmap_update(in_ptr.ptr[i], INVALID);
			}
		}
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
					}
				}
			}
		}
	}
}
