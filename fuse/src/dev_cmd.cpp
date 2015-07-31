/*
 * dev_cmd.cpp
 *
 *  Created on: 2015. 7. 20.
 *      Author: gang
 */

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "debug.h"
#include "dev_cmd.h"

static int write_sync(int argc, char **args)
{
	DMSG("%s", args[1]);
	DfsFile *file = dfsDir.find(args[1]);
	if(!is_file_exist(file))
	{
		DMSG("cannot find %s", args[1]);
		return -ENOENT;
	}

	switch(file->status)
	{
		case DfsFile::REMOVED:
			return -ENOENT;

		case DfsFile::SYNCED:
			// nothing to do
			break;

		case DfsFile::MODIFIED:
		case DfsFile::CREATED:
			return file->upload();
			break;
	}
	return 0;
}

static int clear_cache(int argc, char **args)
{
	DMSG("%s", args[1]);
	DfsFile *file = dfsDir.find(args[1]);
	if(!is_file_exist(file))
	{
		DMSG("cannot find %s", args[1]);
		return -ENOENT;
	}

	switch(file->status)
	{
		case DfsFile::REMOVED:
			// nothing to do
			return 0;

		case DfsFile::SYNCED:
			for(int i = 0; i < MAX_CACHE_BLOCK; i++)
			{
				if(file->cache_block[i]) delete file->cache_block[i];
				file->cache_block[i] = NULL;
			}
			break;

		case DfsFile::MODIFIED:
		case DfsFile::CREATED:
			DMSG("cannot clear modified caches");
			return -1;
	}
	return 0;
}

static int change_blocksize(int argc, char **args)
{
	my_blocksize = atoi(args[1]);
	DMSG("changed block size to %d", my_blocksize);
	return my_blocksize;
}

struct dev_cmd_s {
	const char *cmd;
	int(*func)(int argc, char **args);
	int min_argc;
};


static dev_cmd_s dev_cmd[] = {
		{"sync", write_sync, 2}, // sync <filename>
		{"clear", clear_cache, 2}, // clear <filename>
		{"bsize", change_blocksize, 2}, // bsize <block size>
		{NULL, NULL, 0},
};

static dev_cmd_s *find_dev_cmd(const char *cmd)
{
	int i = 0;
	while(dev_cmd[i].cmd != NULL)
	{
		if(strcmp(dev_cmd[i].cmd, cmd) == 0)
			return &(dev_cmd[i]);
		i++;
	}
	return NULL;
}

int handle_command(char *buf, size_t size, off_t offset)
{
	const char *delim = " \t\n";
	int i;
	char *args[10];

	if(offset != 0) return -2; // do not parse appended data
	if(size < 2) return -3; // at least one character & a null

	args[0] = strtok(buf, delim); // command
	if(args[0] == NULL) return -1;

	// parse
	int argc = 1;
	size_t index = strlen(args[0]) +1;

	for(i=1; i<10; i++)
	{
		args[i] = strtok(NULL, delim);
		if(args[i] != NULL)
		{
			argc++;
			index += strlen(args[i]) + 1;
			if(index >= size) break;
		}
		else break;
	}

	//TEMP for debugging
	for(i=0; i<argc; i++)
	{
		DMSG("args[%d] = \"%s\"", i, args[i]);
	}

	// call cmd function
	dev_cmd_s *cmd = find_dev_cmd(args[0]);
	if(cmd == NULL)
	{
		DMSG("undefind command");
		return -1;
	}

	if(argc < cmd->min_argc) return -1; // too short

	int ret = cmd->func(argc, args);
	if(ret < 0)
	{
		DMSG("ret = %d", ret);
		return ret;
	}
	return size;
}

int read_list(char *buf, size_t size, off_t offset)
{
	const int max_buf = 10240;
	char buffer[max_buf + 256];
	int i;
	flen_t index = 0;
	flen_t s = (flen_t)size;
	flen_t o = (flen_t)offset;

	DMSG("size=%lld offset=%lld", (long long)s, (long long)o);

	sprintf(buffer + index, "(n/k) = (%d/%d), block size = %d, password = \"%s\"\n",
			my_n, my_k, my_blocksize, my_password);
	index += strlen(buffer+index);

	for(i = 0; i < my_n; i++)
	{
		sprintf(buffer + index, "SERVER[%d]: %s\n", i,
				my_server[i]);
		index += strlen(buffer+index);
	}
	sprintf(buffer + index, "\nFILES:\n");
	index += strlen(buffer+index);

	for(i = 0; i < dfsDir.max_file; i++)
	{
		if(dfsDir.files[i])
		{
			sprintf(buffer+index, "[%d] \"%s\" %s \t%lld(%d/%d) \t%s\n", i,
					dfsDir.files[i]->filename,
					dfsDir.files[i]->password,
					dfsDir.files[i]->size,
					dfsDir.files[i]->block_size,
					dfsDir.files[i]->block_enc_size,
					(char *)(dfsDir.files[i]->status_str()));
			//DMSG("%s", buffer+index);
			index += strlen(buffer+index);
			if(index > max_buf) break;
		}
	}

	dev_size = index;
	if(o > index) return 0; // out of bound
	if((o + s) > index) s = index - o;
	strncpy(buf, buffer+o, s);
	//DMSG("read %ld byte : %s", (long)size, buffer+offset);

	return s;
}
