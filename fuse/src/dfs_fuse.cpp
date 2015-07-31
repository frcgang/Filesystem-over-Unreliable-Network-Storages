#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "dfs_file.h"
#include "dfs_exception.h"
#include "debug.h"
#include "parse_conf.h"
#include "dev_cmd.h"

//static int handle_command(char *buf, size_t size, off_t offset);
//static int read_list(char *buf, size_t size, off_t offset);
int dev_size = 100;

DfsDir dfsDir(100);
const char *my_server[] = {
		(char *)"file:///home/gang/workspace/dfs/storage/",
		(char *)"file:///home/gang/workspace/dfs/storage/",
		(char *)"file:///home/gang/workspace/dfs/storage/",
		(char *)"file:///home/gang/workspace/dfs/storage/",
		(char *)"file:///home/gang/workspace/dfs/storage/",
		(char *)"ftp://***REMOVED***/gang/tmp/",
		(char *)"ftp://***REMOVED***@localhost/",
		(char *)"",
		(char *)"",
		(char *)"",
		(char *)"",
		(char *)"",
};

const char *my_password = (char *)"1234";
char *rootdir = (char *)"root.dir";
int my_n = 5;
int my_k = 3;
int my_blocksize = 3072;

bool is_special_path(const char *path)
{
	if(strcmp(path, "/dev") == 0)
	{
		DMSG("special");
		return true;
	}
	else return false;
}

bool is_file_exist(DfsFile *file)
{
	return (file && (file->status != DfsFile::REMOVED));
}

static void *dfs_init(struct fuse_conn_info *conn)
{
	DMSG("INIT DFS FUSE");
	int files = dfsDir.load(rootdir);
	if(files < 0)
		DMSG("cannot load %s", rootdir);
	else DMSG("loaded %d files", files);
	
	return 0;
}

static void dfs_destroy(void *ptr)
{
	DMSG("UNMOUNTING DFS FUSE");
	dfsDir.list();
	int files = dfsDir.save(rootdir);
	if(files < 0)
		DMSG("cannot save %s", rootdir);
	else DMSG("%d files", files);
	return;
}

static int dfs_create(const char *path, mode_t mode, struct fuse_file_info *info)
{
	DMSG("%s", path);
	if(is_special_path(path)) return 0;

	DfsFile *file = dfsDir.find(path + 1);
	if(is_file_exist(file))
		return -EEXIST;

	file = new DfsFile(path + 1, my_password, 
			my_server, my_n, my_k, my_blocksize);
	//file->status = DfsFile::CREATED;
	//char cache_filename[CACHE_DIR_LEN + strlen(path)];
	//file->cache_fp = fopen(file->get_cache_path(cache_filename), "w+");

	dfsDir.add(file);

	return 0;
}

static int dfs_release(const char *path, struct fuse_file_info *info)
{
	DMSG("%s",path);
	if(is_special_path(path)) return 0;

	DfsFile *file = dfsDir.find(path + 1);
	if(!is_file_exist(file))
		return -ENOENT;

	file->release();

	return 0;
}

static int dfs_write(const char *path, const char *buf, size_t size, off_t offset,
		struct fuse_file_info *info)
{
	DMSG("%s(%ld)",path,(long)size);
	//int w_size = 0;

	if(is_special_path(path))
	{
		return handle_command((char *)buf, size, offset);
	}

	DfsFile *file = dfsDir.find(path + 1);
	if(!is_file_exist(file))
		return -ENOENT;

	switch(file->status)
	{
		case DfsFile::REMOVED:
			return -ENOENT;

		case DfsFile::SYNCED:
			file->status = DfsFile::MODIFIED;
			/* no break */

		case DfsFile::MODIFIED:
		case DfsFile::CREATED:
			DMSG("offset: %ld", (long)offset);
			return file->write(buf, size, offset);
	}

	DMSG("unreachable");
	return -1;
}

/*
static int dfs_rename(const char *path, const char *new_path)
{
	return 0;
}
*/

static int dfs_unlink(const char *path)
{
	DMSG("%s", path);
	if(is_special_path(path)) return -1;

	//DfsFile *file = dfsDir.find(path + 1);
	return dfsDir.unlink(path + 1);
}

static int dfs_getattr(const char *path, struct stat *stbuf)
{
	DMSG("%s", path);
	int res = 0;
	DfsFile *file = dfsDir.find(path+1);

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0700;
		stbuf->st_nlink = 2;
	} else if (is_special_path(path)) {
		stbuf->st_mode = S_IFREG | 0700; // write only
		stbuf->st_nlink = 1;
		stbuf->st_size = dev_size;
	} else if (is_file_exist(file) && (strcmp(path+1, file->filename) == 0)) {
		stbuf->st_mode = S_IFREG | 0700;
		stbuf->st_nlink = 1;
		stbuf->st_size = file->size;
	} else {
		DMSG("cannot find %s", path);
		res = -ENOENT;
	}

	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();

	return res;
}

static int dfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	DMSG("%s",path);
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

#ifdef DEBUG
	//dfsDir.list();
#endif

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	for(int i = 0; i < dfsDir.max_file; i++)
	{
		DfsFile *file = dfsDir.files[i];
		if(file) switch(file->status)
		{
			case DfsFile::SYNCED:
			case DfsFile::MODIFIED:
			case DfsFile::CREATED:
				DMSG("filling %s", dfsDir.files[i]->filename);
				filler(buf, dfsDir.files[i]->filename, NULL, 0);
				break;
			default:
				break;
		}
	}

	filler(buf, "dev", NULL, 0);
	return 0;
}

static int dfs_open(const char *path, struct fuse_file_info *fi)
{
	DMSG("%s",path);
	if(is_special_path(path)) return 0;

	DfsFile *file = dfsDir.find(path + 1);
	if(!is_file_exist(file))
	{
		if((fi->flags & 3) == O_RDONLY) return -ENOENT;
		DMSG("create %s", path);

		file = dfsDir.add(new DfsFile(path+1, my_password, my_server, my_n, my_k, my_blocksize));
		//file->status = DfsFile::CREATED;
		if(file == NULL) return -ENOENT;
	}

	//if ((fi->flags & 3) != O_RDONLY)
	//	return -EACCES;

	switch(file->status)
	{
		case DfsFile::REMOVED:
			return -ENOENT;

		default:
			//const char *mode = "";
			switch (fi->flags & 3)
			{
				case O_RDONLY:
				//	mode = "r";
					break;
				case O_WRONLY:
				case O_RDWR:
				//	mode = "r+";
					break;
				default:
					return -1; // unreachalbe
			}
			DMSG("flags & 3 = %d", (fi->flags & 3));
			//DMSG("%x %x %x", O_RDONLY, O_WRONLY, O_RDWR);

			//DMSG("opening mode %s %x", mode, fi->flags);
			//char cache_filename[CACHE_DIR_LEN + strlen(path)];
			//file->cache_fp = fopen(file->get_cache_path(cache_filename), mode);
	}

	return 0;
}

static int dfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	DMSG("%s",path);
	int r_size = 0;

	if (is_special_path(path))
	{
		return read_list(buf, size, offset);
	}

	DfsFile *file = dfsDir.find(path + 1);
	if(!is_file_exist(file))
		return -ENOENT;

	switch(file->status)
	{
		case DfsFile::REMOVED:
			return -ENOENT;

		default:
			r_size = file->read(buf, size, offset);
			DMSG("read %d bytes from offset %ld", r_size, (long)offset);
			break;
	}

	return r_size;
}

static int dfs_truncate(const char *path, off_t offset)
{
	DMSG("%s", path);
	if(is_special_path(path)) return 0;

	DfsFile *file = dfsDir.find(path + 1);
	if(!is_file_exist(file))
		return -ENOENT;

	return file->truncate(offset);
}

// aux commands




/*
static struct fuse_operations dfs_oper = {
	getattr	: dfs_getattr,
	.readlink	= NULL,
	.mknod	= NULL,
	.mkdir	= NULL,
	//unlink		: dfs_unlink,
	.rmdir	= NULL,
	.symlink	= NULL,
	.rename	= NULL,//		= dfs_rename,
	.link	= NULL,
	.chmod	= NULL,
	.chown	= NULL,
	.truncate	= NULL,
	.open		= dfs_open,
	.read		= dfs_read,
	.write		= dfs_write,
	.statfs	= NULL,
	.flush	= NULL,
	.release	= dfs_release,
	.fsync	= NULL,
	.setxattr	= NULL,
	.getxattr	= NULL,
	.listxattr	= NULL,
	.removexattr	= NULL,
	.opendir	= NULL,
	.readdir	= dfs_readdir,
	.fsyncdir	= NULL,
	.init		= dfs_init,
	.destroy	= dfs_destroy,
	.access	= NULL,
	.create	= NULL, //= dfs_create
	.ftruncate = NULL,
		.fgetattr = NULL,
		.lock = NULL,
		.utimens = NULL,
		.bmap = NULL,
		.ioctl = NULL,
		.poll = NULL,
		.write_buf = NULL,
		.read_buf = NULL,
		.flock = NULL,
		.fallocate= NULL,
	// utimens, truncate, flush,= NULL,
};
		*/

#ifdef TEST_FUSE
int main(int argc, char *argv[])
{
	static struct fuse_operations dfs_oper;
	bzero(&dfs_oper, sizeof(dfs_oper));
	dfs_oper.getattr	= dfs_getattr;
	dfs_oper.readdir	= dfs_readdir;
	dfs_oper.open		= dfs_open;
	dfs_oper.read		= dfs_read;
	dfs_oper.init		= dfs_init;
	dfs_oper.destroy	= dfs_destroy;
	dfs_oper.release	= dfs_release;
	dfs_oper.write		= dfs_write;
	dfs_oper.unlink		= dfs_unlink;
	dfs_oper.create		= dfs_create;
	dfs_oper.truncate	= dfs_truncate;

	try
	{
		DfsConf conf("dfs.conf");

		my_blocksize = conf.get_int("blocksize");
		my_n = conf.count("server");
		my_k = my_n/2 +1;
		DMSG("CONFIGURE: (%d/%d) block size = %d", my_n, my_k, my_blocksize);
		for(int i = 0; i < my_n; i++)
		{
			my_server[i] = conf.get("server", i);
			DMSG("SERVER: %s", my_server[i]);
		}

		return fuse_main(argc, argv, &dfs_oper, NULL);
	}
	catch(DfsException *e)
	{
		fprintf(stderr, "ERROR: %s\n", e->msg);
		delete e;
	}
	return 0;
}
#endif
