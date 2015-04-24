/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` fusexmp.c -o fusexmp `pkg-config fuse --libs`

  Note: This implementation is largely stateless and does not maintain
        open file handels between open and release calls (fi->fh).
        Instead, files are opened and closed as necessary inside read(), write(),
        etc calls. As such, the functions that rely on maintaining file handles are
        not implmented (fgetattr(), etc). Those seeking a more efficient and
        more complete implementation may wish to add fi->fh support to minimize
        open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include "aes-crypt.h"
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
struct p_data {
	char* userkey;
	char* mirrordir;
};
struct p_data * userData;

static void xmp_getpath(char* fpath, const char* path) {
	userData =(struct p_data*) fuse_get_context()->private_data;
	strcpy(fpath, userData -> mirrordir);
	strcat(fpath, path);
}

static void xmp_getkey(char* mykey) {
	userData = (struct p_data*) fuse_get_context()->private_data;
	strcpy(mykey, userData -> userkey);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = lstat(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = access(fpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = readlink(fpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	char fpath[4096];
	xmp_getpath(fpath, path);

	(void) offset;
	(void) fi;

	dp = opendir(fpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fpath, mode);
	else
		res = mknod(fpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = mkdir(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = unlink(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = rmdir(fpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = chmod(fpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = lchown(fpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = truncate(fpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char fpath[4096];
	char dpath[4096];
	char key[4096];
	
	uid_t curr_id;
	char attr[4096];
	char uname[4096];
	curr_id = getuid();
	strcpy(attr, "user.");
	sprintf(uname, "%d", curr_id);
	strcat(attr, uname);
	
	FILE* myfile;
	FILE* file2;
	ssize_t xsize;
	xmp_getpath(fpath, path);
	xmp_getpath(dpath, "decrypted.e");
	xsize = getxattr(fpath, attr, NULL, 0);
	
	if (xsize >= 0) {
		FILE* infile;
		FILE* outfile;
		char inpath[4096];
		char outpath[4096];
		char deckey[4096];
		xmp_getkey(key);
		xmp_getpath(inpath, "infile.txt");
		xmp_getpath(outpath, "outfile.e");
		infile = fopen(inpath, "wb+");
		outfile = fopen(outpath, "wb+");
		char lst[4096];
		getxattr(fpath, attr, lst, 0);
		fprintf(infile, "%s", lst);
		fclose(infile);
		infile = fopen(inpath, "rb+");
		do_crypt(infile, outfile, 0, key);
		fclose(infile);
		fclose(outfile);
		outfile = fopen(outpath, "rb+");
		fgets(deckey, 4096, outfile);
		fclose(outfile);
		remove(inpath);
		remove(outpath);
		
		
		myfile = fopen(fpath, "rb");
		file2 = fopen(dpath, "wb+");
		do_crypt(myfile, file2, 0, deckey);
		fclose(myfile);
		fclose(file2);
		myfile = fopen(fpath, "wb+");
		file2 = fopen(dpath, "rb");
		do_crypt(file2, myfile, -1, deckey);
		fclose(myfile);
		fclose(file2);
		remove(dpath);
	}
	

	res = open(fpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	(void) fi;
	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	(void) fi;
	fd = open(fpath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char fpath[4096];
	xmp_getpath(fpath, path);

	res = statvfs(fpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;

    int res;
    char fpath[4096];
    char inpath[4096];
    char outpath[4096];
    char* master = "23847659273";
    FILE* infile;
    FILE* outfile;
	xmp_getpath(fpath, path);
	xmp_getpath(inpath, "infile.txt");
	xmp_getpath(outpath, "outfile.e");
    res = creat(fpath, mode);
    if(res == -1)
	return -errno;

    close(res);
    
    infile = fopen(inpath, "wb+");
    fprintf(infile, "%s", master);
    fclose(infile);
    
    infile = fopen(inpath, "rb+");
    outfile = fopen(outpath, "wb+");
    char key[4096];
    char enckey[4096];
    xmp_getkey(key);
    
    do_crypt(infile, outfile, 1, key);
    fclose(outfile);
    outfile = fopen(outpath, "rb+");
	fgets(enckey, 4096, outfile);
	fclose(infile);
	fclose(outfile);
	remove(inpath);
	remove(outpath);
    
    uid_t curr_id;
	char attr[4096];
	char uname[4096];
	
	curr_id = getuid();
	strcpy(attr, "user.");
	sprintf(uname, "%d", curr_id);
	strcat(attr, uname);
	int size;
	for(size = 0; enckey[size] != '\0'; size++);
	setxattr(fpath, attr, enckey, size, 0);
    

    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	char fpath[4096];
	char dpath[4096];
	char key[4096];
	FILE* myfile;
	FILE* file2;
	ssize_t xsize;
	xmp_getpath(fpath, path);
	xmp_getpath(dpath, "encrypted.e");
	
	uid_t curr_id;
	char attr[4096];
	char uname[4096];
	curr_id = getuid();
	strcpy(attr, "user.");
	sprintf(uname, "%d", curr_id);
	strcat(attr, uname);
	
	xsize = getxattr(fpath, attr, NULL, 0);
	if (xsize >= 0) {
		FILE* infile;
		FILE* outfile;
		char inpath[4096];
		char outpath[4096];
		char deckey[4096];
		xmp_getkey(key);
		xmp_getpath(inpath, "infile.txt");
		xmp_getpath(outpath, "outfile.e");
		infile = fopen(inpath, "wb+");
		outfile = fopen(outpath, "wb+");
		char lst[4096];
		getxattr(fpath, attr, lst, 0);
		fprintf(infile, "%s", lst);
		fclose(infile);
		infile = fopen(inpath, "rb+");
		do_crypt(infile, outfile, 0, key);
		fclose(infile);
		fclose(outfile);
		outfile = fopen(outpath, "rb+");
		fgets(deckey, 4096, outfile);
		fclose(outfile);
		remove(inpath);
		remove(outpath);
		
		myfile = fopen(fpath, "rb");
		file2 = fopen(dpath, "wb+");
		do_crypt(myfile, file2, 1, deckey);
		fclose(myfile);
		fclose(file2);
		myfile = fopen(fpath, "wb+");
		file2 = fopen(dpath, "rb");
		do_crypt(file2, myfile, -1, deckey);
		fclose(myfile);
		fclose(file2);
		remove(dpath);
	}

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char fpath[4096];
	xmp_getpath(fpath, path);
	if(!strcmp("e", value)){
		char inpath[4096];
		char outpath[4096];
		char* master = "23847659273";
		FILE* infile;
		FILE* outfile;
		xmp_getpath(inpath, "infile.txt");
		xmp_getpath(outpath, "outfile.e");
		
		infile = fopen(inpath, "wb+");
		fprintf(infile, "%s", master);
		fclose(infile);
		
		infile = fopen(inpath, "rb+");
		outfile = fopen(outpath, "wb+");
		char key[4096];
		char enckey[4096];
		xmp_getkey(key);
		
		do_crypt(infile, outfile, 1, key);
		fclose(outfile);
		outfile = fopen(outpath, "rb+");
		fgets(enckey, 4096, outfile);
		fclose(infile);
		fclose(outfile);
		remove(inpath);
		remove(outpath);
		
		xmp_getpath(fpath, path);
		int sizel;
		for(sizel = 0; enckey[sizel] != '\0'; sizel++);
		lsetxattr(fpath, name, enckey, sizel, 0);
	}
	else {
		int res = lsetxattr(fpath, name, value, size, flags);
		if (res == -1)
			return -errno;
	}
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char fpath[4096];
	xmp_getpath(fpath, path);
	int res = lgetxattr(fpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char fpath[4096];
	xmp_getpath(fpath, path);
	int res = llistxattr(fpath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char fpath[4096];
	xmp_getpath(fpath, path);
	int res = lremovexattr(fpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	//keyphrase 1, mirror 2, mount 3
	char *nargv[2];
	nargv[0] = argv[0];
	nargv[1] = argv[3];
	
	struct p_data* my_args = malloc(sizeof(struct p_data));
	my_args -> userkey = argv[1];
	my_args -> mirrordir = realpath(argv[2], NULL);
	
	umask(0);
	argc -= 2;
	return fuse_main(argc, nargv, &xmp_oper, my_args);
}
