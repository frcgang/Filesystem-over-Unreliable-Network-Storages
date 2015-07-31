#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "dfs_file.h"
#include "dfs_exception.h"
#include "debug.h"

#ifndef ERANGE
#define ERANGE 34
#endif

DfsBlock::DfsBlock(DfsFile *file, int index)
{
	DMSG("%d", index);
	this->file = file;
	this->index = index;
	this->status = (file->last_block_index < index) ? MODIFIED : NOTDOWNLOADED;
	cache_fp = NULL;
}

DfsBlock::~DfsBlock()
{
	close_cache_fp();
	DMSG("unlink %s", get_cache_path());
	unlink(get_cache_path());
}

void DfsBlock::close_cache_fp()
{
	if(cache_fp != NULL)
	{
		DMSG("close FILE");
		fclose(cache_fp);
		cache_fp = NULL;
	}
}

int DfsBlock::upload()
{
	DMSG("%s / %d", file->filename, index);
	close_cache_fp();

	const char *filename = get_cache_path();
	int enc_size = upload_file(filename, file->password, file->server, file->n, file->k);
	if(enc_size > 0) status = SYNCED;

	return enc_size;
}

int DfsBlock::download()//bool is_last_block)
{
	if(status != NOTDOWNLOADED)
	{
		DMSG("status=%d", status);
		return 0; // already, we have
	}
	DMSG("%s / %d", file->filename, index);

	int enc_size = (file->last_block_index == index) ?
			file->last_enc_size : file->block_enc_size;
	DMSG("enc size = %d / %d %d %d %d", enc_size,
			file->last_block_index , index,
			file->last_enc_size , file->block_enc_size);
	close_cache_fp();

	const char *filename = get_cache_path();
	int size = download_file(filename,
			enc_size,
			file->password, file->server, file->n, file->k);
	if(size > 0)
	{
		DMSG("%s %d synced size %d", file->filename, index, size);
		status = SYNCED;
	}
	else DMSG("%s %d sync failure", file->filename, index);

	return size;
}

const char *DfsBlock::get_cache_path(char *buffer)
{
	static char tmp_buf[4096];
	if(buffer == NULL) buffer = tmp_buf;
		//buffer = new char[CACHE_DIR_LEN + strlen(file->filename) + CACHE_INDEX_LEN + 1];
	char folder[3] = "/0";
	folder[1] = '0' + (char)(index % 10);
	sprintf(buffer, CACHE_DIR "%s/%s_b%06d", folder, file->filename, index);
	return buffer;
}

const char *DfsBlock::get_cache_path()
{
	return get_cache_path(NULL);
}

int DfsBlock::open()
{
	DMSG("");
	if(cache_fp) DMSG("WARNING: fp not null");

	cache_fp = fopen(get_cache_path(), "r+");
	if(cache_fp == NULL)
	{
		DMSG("create");
		cache_fp = fopen(get_cache_path(), "w+");
		if(!cache_fp)
		{
			DMSG("cannot open cache block file");
			return -1;
		}
	}

	DMSG("opened");
	return 0;
}

int DfsBlock::release()
{
	close_cache_fp();
	return 0;
}

flen_t DfsBlock::read(char *buf, flen_t size, flen_t offset)
{
	DMSG("reading %lld bytes from %lld", size, offset);
	flen_t left = size;
	char *ptr = buf;
	int max_try = 100;
	flen_t off = offset;

	// download the block
	if(status == DfsBlock::NOTDOWNLOADED)
		download();

	if(cache_fp == NULL)
	{
		if(open() < 0) return -1;
	}

	DMSG("left = %lld, max_try=%d",left, max_try);
	while(left && max_try)
	{
		fseek(cache_fp, off, SEEK_SET);
		int r = fread(ptr, 1, size, cache_fp);
		DMSG("get %d bytes", r);
		if(feof(cache_fp)) break;
		ptr += r;
		off += r;
		left -= r;
		max_try--;
	}
	if(!max_try) { DMSG("fail"); return -1; } // fail
	return (size-left);
}

flen_t DfsBlock::write(const char *buf, flen_t size, flen_t offset)
{
	DMSG("writing %lld bytes", size);
	int left = size;
	flen_t off = offset;
	const char *ptr = buf;
	int max_try = 100;

	// download the block
	if(status == DfsBlock::NOTDOWNLOADED)
		download();

	if(cache_fp == NULL)
	{
		if(open() < 0) return -1;
	}

	while(left && max_try)
	{
		DMSG("trying %d at %lld", left, off);
		fseek(cache_fp, off, SEEK_SET);
		int w = fwrite(ptr, 1, left, cache_fp);
		DMSG("written %d", w);
		ptr += w;
		left -= w;
		off += w;
		max_try--;
	}
	if(!max_try) { DMSG("fail"); return -1; } // fail
	return size;
}

int DfsBlock::truncate(flen_t offset)
{
	DMSG("%lld",offset);
	if(cache_fp == NULL)
		if(open() < 0)
		{
			DMSG("cannot open");
			return -1;
		}
	DMSG("now truncate %lld", offset);
	int ret= ftruncate(fileno(cache_fp), offset);
	if(ret < 0) perror("truncate error");
	else DMSG("no truncate error");
	return ret;
}

/*int DfsBlock::unlink()
{
	DMSG("");
	close_cache_fp();
	return unlink(get_cache_path());
}*/
//////////////////////
// DfsFile
//////////////////////

// create the new local file
DfsFile::DfsFile(const char *filename, const char *password,
		const char **server, int n, int k, int block_size)
{
	this->filename = strdup(filename);
	this->password = password;
	this->server = server;
	this->n = n;
	this->k = k;
	this->block_size = block_size;
	need_to_clear = false;

	bzero(cache_block, sizeof(cache_block));
	size = 0;
	//num_cache_block = 0;
	status = CREATED;
	block_enc_size = 0;
	last_block_index = -1; // no remote last block
	last_enc_size = 0;

	pthread_mutex_init(&lock, NULL);
}

// from dir file : filename password size enc_size n k servers..
static char *get_a_word(char **buf)
{
	char *ptr = strchr(*buf, ' ');
	*ptr = '\0';
	char *word = strdup(*buf);
	*buf = ptr + 1;

	//DMSG("read '%s'", word);
	return word;
}

static long get_a_long(char **buf)
{
	char *ptr = strchr(*buf, ' ');
	if(ptr != NULL) *ptr = '\0';
	long value = atol(*buf);
	*buf = ptr + 1;

	//DMSG("read (%lld)", value);
	return value;
}

DfsFile::DfsFile(FILE *dir_fp)
{
	char buffer[1000];
	if(fgets(buffer, 1000, dir_fp) == NULL) throw new DfsException("no more entry");
	DMSG("read: \"%s\"", buffer);
	int len = strlen(buffer);
	if(len < 5) throw new DfsException("end of file");
	char *ptr = buffer;

	filename = get_a_word(&ptr);
	password = get_a_word(&ptr);
	size = get_a_long(&ptr);
	block_size = get_a_long(&ptr);
	block_enc_size = get_a_long(&ptr);
	last_block_index = (size - 1)/ block_size;
	last_enc_size = get_a_long(&ptr);
	n = (int)get_a_long(&ptr);
	k = (int)get_a_long(&ptr);
	server = new const char *[n];
	status = SYNCED;
	for(int i = 0; i < n; i++)
		server[i] = get_a_word(&ptr);

	need_to_clear = true;
	pthread_mutex_init(&lock, NULL);
}


DfsFile::~DfsFile()
{
	for(int i = 0; i < MAX_CACHE_BLOCK; i++)
	{
		if(cache_block[i]) delete cache_block[i];
	}

	free((void *)filename); // filename should be freed after clearing cache blocks

	if(need_to_clear)
	{
		free((void *)password);
		for(int i = 0; i < n; i++)
			free((void *)server[i]);
		delete[] server;
	}
}

flen_t DfsFile::upload(flen_t offset, flen_t count)
{
	int start_index = offset / block_size;
	int stop_index = (offset+count-1)/block_size;
	if(stop_index >= get_num_cache_blocks()) return -ERANGE;

	STAMP("UPLOADING %s : from %lld size %lld", this->filename, offset, count);
	int sum = 0;
	for(int i = start_index; i <= stop_index; i++)
	{
		if(cache_block[i] == NULL)
		{
			DMSG("error : cache_block[%d] is null", i);
			return -1;
		}

		/*switch(cache_block[i]->status)
		{
		case UNUSED:
		case NOTDOWNLOADED:
		case SYNCED:
			continue; // skip
		case MODIFIED:
			break; // go on
		}*/
		if(cache_block[i]->status != DfsBlock::MODIFIED) continue; //skip

		int size = cache_block[i]->upload();
		if(size < 0)
		{
			DMSG("error : cache_block->upload returns %d", size);
			return -1;
		}
		if(i == (get_num_cache_blocks() - 1)) // last block
			last_enc_size = size;
		else
		{
			if(block_enc_size == 0) block_enc_size = size;
			else if(size != block_enc_size)
			{
				DMSG("WARNING: size %d differs enc size", size);
			}
		}
		sum += size;
	}
	if(stop_index > last_block_index) last_block_index = stop_index;

	STAMP("UPLOADED %s", this->filename);
	uploaded = true;
	return sum;
}

flen_t DfsFile::download(flen_t count, flen_t offset)
{
	DMSG("count=%lld, offset=%lld", count, offset);
	int start_index = offset / block_size;
	int stop_index = (offset+count-1)/block_size;

	flen_t sum = 0;
	for(int i = start_index; i <= stop_index; i++)
	{
		if(cache_block[i] == NULL) cache_block[i] = new DfsBlock(this, i);
		int size = cache_block[i]->download();
		if(size < 0)
		{
			DMSG("error : cache_block->download returns %d", size);
			return -1;
		}
//		if((i < (num_cache_block - 1)) // last block
//				&& (size != block_size))
//		{
//			DMSG("WARNING: %d is not block size", size);
//		}
		sum += size;
	}

	return sum;
}

int DfsFile::save(FILE *dir_fp)
{
	// filename password size enc_size n k servers... 
	if(fprintf(dir_fp, "%s %s %lld %d %d %d %d %d ", filename, password,
			size, block_size,
			block_enc_size,last_enc_size,
			n, k) < 0) return -1;
	for(int i = 0; i < n; i++)
		if(fprintf(dir_fp, "%s ", server[i]) <0) return -1;
	if(fprintf(dir_fp, "\n") < 0) return -1;
	return 0;
}

/*char *DfsFile::get_cache_path(char *buffer)
{
	sprintf(buffer, CACHE_DIR "/%s", filename);
	return buffer;
}*/

flen_t DfsFile::read(char *buf, flen_t size, flen_t offset)
{
	if(offset >= this->size) return 0;
	if(offset+size > this->size) size = this->size - offset;
	DMSG("size=%lld + %lld / %lld", offset, size, this->size);
	flen_t left = size;
	char *ptr = buf;
	flen_t to_read = block_size - offset % block_size;
	if(to_read > size) to_read = size;
	int block_index = offset / block_size;
	flen_t block_offset = offset % block_size;

	//download(size, offset);

	// LOCK handling of concurrency of cache blocks
	//////// CRITICAL REGION : BEGIN
	DMSG("(%s: %lld - %lld) WAIT LOCK", this->filename, offset, (offset+size));
	pthread_mutex_lock(&lock);
	DMSG("(%s: %lld - %lld) LOCK OWNED", this->filename, offset, (offset+size));

	while(left>0)
	{
		if(cache_block[block_index] == NULL)
		{
			DMSG("make blokc %d", block_index);
			cache_block[block_index] = new DfsBlock(this, block_index);
		}
		DMSG("%d %d", cache_block[block_index]->index,
				cache_block[block_index]->status);

		flen_t r = cache_block[block_index]->read(ptr, to_read, block_offset);
		DMSG("read %lld/%lld from %d block", r, to_read, block_index);
		if(r < 0){ DMSG("fail"); pthread_mutex_unlock(&lock); return r; } // error
		left -= r;
		ptr += r;
		if(r != to_read){
			DMSG("EOF? %lld != %lld", r, to_read);
			break;
		}
		block_index++;
		block_offset = 0;
		to_read = (left > block_size) ? block_size : left;
	}

	DMSG("(%s: %lld - %lld) UNLOCK", this->filename, offset, (offset+size));
	pthread_mutex_unlock(&lock);

	return size - left;
}

flen_t DfsFile::write(const char *buf, flen_t size, flen_t offset)
{
	DMSG("size=%lld", size);
	int left = size;
	const char *ptr = buf;
	flen_t to_write = block_size - offset % block_size;
	if(to_write > size) to_write = size;
	int block_index = offset / block_size;
	flen_t block_offset = offset % block_size;
	while(left>0)
	{
		if(cache_block[block_index] == NULL)
		{
			DMSG("make block %d", block_index);
			cache_block[block_index] = new DfsBlock(this, block_index);
		}
		flen_t w = cache_block[block_index]->write(ptr, to_write, block_offset);
		if(w < 0){ DMSG("fail"); return w; } // error
		if(w != to_write){ DMSG("WARNING : %lld != %lld", w, to_write); }
		cache_block[block_index]->status = DfsBlock::MODIFIED;
		ptr += w;
		left -= to_write;
		block_index++;
		block_offset = 0;
		to_write = (left > block_size) ? block_size : left;
	}

	if(this->size < (size + offset)) this->size = (size+offset);
	return size;
}

void DfsFile::release()
{
	int num_blocks = get_num_cache_blocks();
	DMSG("%d blocks", num_blocks);
	for(int i = 0; i < num_blocks; i++)
	{
		if(cache_block[i]) cache_block[i]->release();
	}
}

int DfsFile::unlink()
{
	DMSG("%s", filename);
	int num_blocks = get_num_cache_blocks();
	for(int i = 0; i < num_blocks; i++)
	{
		if(cache_block[i]) delete cache_block[i];
	}
	status = REMOVED;
	return 0;
}

int DfsFile::truncate(flen_t offset)
{
	DMSG("%lld", offset);
	int num_blocks = get_num_cache_blocks();
	int truncate_block_index = offset / block_size + 1;

	for(int i = truncate_block_index; i < num_blocks; i++)
	{
		if(cache_block[i])
		{
			DMSG("delete block %d", i);
			delete cache_block[i];
			cache_block[i] = NULL;
		}
	}

	// for last block
	int last = offset / block_size;
	if(cache_block[last] == NULL)
	{
		cache_block[last] = new DfsBlock(this, last);
	}
	DMSG("truncate block %d to offset %lld", last, offset%block_size);

	if(size > offset) size = offset;
	return cache_block[last]->truncate(offset % block_size);
}

const char *DfsFile::status_str()
{
	switch(status)
	{
		case SYNCED: return "synced";
		case MODIFIED: return "modified";
		case CREATED: return "created";
		case REMOVED: return "removed";
	}
	return "";
}


///////////////////
// DfsDir
///////////////////

DfsDir::DfsDir(int max_file)
{
	files = new DfsFile *[max_file];
	this->max_file = max_file;
	for(int i = 0; i < max_file; i++)
	{
		files[i] = NULL;
	}
}

DfsDir::~DfsDir()
{
	for(int i = 0; i < max_file; i++)
	{
		if(files[i] != NULL) delete files[i];
	}
	delete[] files;
}

DfsFile *DfsDir::find(const char *filename)
{
	for(int i = 0; i < max_file; i++)
	{
		if(files[i] && (strcmp(files[i]->filename, filename) == 0))
		{
			DMSG("found %s at %d status=%d", filename, i, files[i]->status);
			return files[i];
		}
	}
	return NULL;
}

DfsFile *DfsDir::add(DfsFile *file)
{
	int i;
	// find empty
	for(i = 0; i < max_file; i++)
	{
		if(files[i] == NULL)
		{
			files[i] = file;
			break;
		}
	}

#ifdef DEBUG
	list();
#endif

	if(i == max_file) return NULL;
	else return file;
}

int DfsDir::save(const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if(fp == NULL) return -1;
	int i;

	for(i=0; i<max_file; i++)
	{
		if(files[i])
		{
			switch(files[i]->status)
			{
				case DfsFile::SYNCED:
					// nothing to do
					break;

				case DfsFile::MODIFIED:
				case DfsFile::CREATED:
					files[i]->upload();
					break;

				case DfsFile::REMOVED:
					// TODO: remove
					break;
			}
			if(files[i]->status != DfsFile::REMOVED)
				files[i]->save(fp);
		}
	}

	return i;
}

int DfsDir::load(const char *filename)
{
	DMSG("%s", filename);
	FILE *fp = fopen(filename, "r");
	if(fp == NULL)
	{
		DMSG("cannot open %s",filename);
		return -1;
	}
	int i;

	for(i=0; i<max_file; i++)
	{
		//DfsFile *file;
		try
		{
			files[i] = new DfsFile(fp);
			files[i]-> status = DfsFile::SYNCED;
			DMSG("loading %s",files[i]->filename);
		}
		catch(DfsException *e)
		{
			DMSG("Exception: %s", e->msg);
			delete e;
			break;
		}

	}

	fclose(fp);
	return i;
}

int DfsDir::unlink(const char *filename)
{
	DMSG("%s", filename);
	int i;
	for(i = 0; i < max_file; i++)
	{
		if(files[i] && (strcmp(files[i]->filename, filename) == 0))
		{
			DMSG("found %s at %d status=%d", filename, i, files[i]->status);
			break;
		}
	}
	if(i == max_file) return -ENOENT;

	if(files[i]->status == DfsFile::CREATED)
	{
		delete files[i];
		files[i] = NULL;
	}
	else
	{
		files[i]->unlink(); // REMOVED
	}

	return 0;
}
void DfsDir::list()
{
	DMSG("listing files");
	for(int i = 0; i<max_file; i++)
	{
		if(files[i])
		{
			fprintf(stderr, "[%s] %lld %s", files[i]->filename, files[i]->size,
					(char *)files[i]->status_str());
			fprintf(stderr,"\n");
		}
	}
}

#ifdef TEST_DFS_FILE

int main(int argc, char **argv)
{
	return 0;
}
#endif
