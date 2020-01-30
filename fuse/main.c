/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall koofs.c `pkg-config fuse3 --cflags --libs` -o koofs
 *
 * ## Source code ##
 * \include koofs.c
 */


#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include "fs_dir.h"
#include "fs_file.h"
#include "fs_generic.h"

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
static struct options {
	const char *filename;
	const char *contents;
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("--name=%s", filename),
	OPTION("--contents=%s", contents),
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

#ifdef __cplusplus
extern "C" {
#endif
struct fuse_operations koofs_oper = {
	.getattr	= fs_getattr,
	.readlink	= fs_readlink,
	.mkdir		= fs_mkdir,
	.unlink     = fs_unlink,
	.rmdir      = fs_rmdir,
	.symlink	= fs_symlink,
	.rename     = fs_rename,
	.chmod      = fs_chmod,
	.chown      = fs_chown,
	.truncate   = fs_truncate,
	.open		= fs_open,
	.read		= fs_read,
	.write      = fs_write,
	.flush      = fs_flush,
	.release    = fs_release,
	.fsync		= fs_fsync,
	.opendir    = fs_opendir,
	.readdir	= fs_readdir,
	.releasedir = fs_releasedir,
	.init       = fs_init,
	.destroy    = fs_destroy,
	.access     = fs_access,
	.create     = fs_create,              // mknod + open
	.utimens	= fs_utimens,
};

#ifdef __cplusplus
}
#endif

static void show_help(const char *progname)
{
	printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --name=<s>          Name of the \"koofs\" file\n"
	       "                        (default: \"koofs\")\n"
	       "    --contents=<s>      Contents \"koofs\" file\n"
	       "                        (default \"koofs, World!\\n\")\n"
	       "\n");
}

int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	/* Set defaults -- we have to use strdup so that
	   fuse_opt_parse can free the defaults if other
	   values are specified */
	options.filename = strdup("koofs");
	options.contents = strdup("koofs World!\n");

	/* Parse options */
	if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
		return 1;

	/* When --help is specified, first print our own file-system
	   specific help text, then signal fuse_main to show
	   additional help (by adding `--help` to the options again)
	   without usage: line (by setting argv[0] to the empty
	   string) */
	if (options.show_help) {
		show_help(argv[0]);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0] = (char*) "";
	}

	ret = fuse_main(args.argc, args.argv, &koofs_oper, NULL);
	fuse_opt_free_args(&args);
	return ret;
}
