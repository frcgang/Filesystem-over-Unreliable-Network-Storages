#ifndef _DFS_FILE_H
#define _DFS_FILE_H

#include <stdio.h>
#include <pthread.h>

#define CACHE_DIR "cache"
#define CACHE_DIR_LEN 6 // +1 for '\0'
#define CACHE_INDEX_LEN 8 // _b000000

#define MAX_CACHE_BLOCK 10000

typedef long long flen_t;
//#define FLEN_T_FORMAT "%lld"

class DfsFile;
class DfsBlock
{
public:
	enum BlockStatus
	{
		//UNUSED,
		NOTDOWNLOADED,
		SYNCED,
		MODIFIED,
	};

	DfsFile *file;
	BlockStatus status;
	int index;

	DfsBlock(DfsFile *file, int index);
	~DfsBlock();

	int upload(); // returns encrypted length
	int download(); //bool is_last_block = false); // returns decrypted length
	//int download_last() { return download(true); }

	int open();
	int release();
	flen_t read(char *buf, flen_t size, flen_t offset);
	flen_t write(const char *buf, flen_t size, flen_t offset);
	int truncate(flen_t offset);
	//int unlink();

private:
	FILE *cache_fp;

	void close_cache_fp();
	const char *get_cache_path(char *buffer);
	const char *get_cache_path(); // static char[]

};

class DfsFile
{
	private:
		bool need_to_clear;
		pthread_mutex_t lock;

	public:
		enum FileStatus
		{
			//CLOSED,
			//OPENED,
			SYNCED,
			MODIFIED,
			CREATED,
			REMOVED,
		};
	
		const char *filename;
		const char *password;

		const char **server;
		int n;
		int k;

		flen_t size;
		int block_size; // number of bytes in each block
		int block_enc_size;

		int last_block_index;
		int last_enc_size;

		FileStatus status;

		DfsBlock *cache_block[MAX_CACHE_BLOCK];

		//FILE *cache_fp; // local cache file I/O

		DfsFile(const char *filename, const char *password,
				const char **server, int n, int k, int block_size);
				// create local file: status=CREATED
		DfsFile(FILE *dir_fp); // read a entry from the root dir file
		~DfsFile();

		flen_t upload(flen_t offset, flen_t count);
		flen_t upload(){ flen_t s = upload((flen_t)0, size); if(s>=0) status = SYNCED; return s; } // commit
		flen_t download(flen_t count, flen_t offset);
		int save(FILE *dir_fp);
		int get_num_cache_blocks() { return ((size - 1)/ block_size + 1); }
		const char *status_str();

		// FILE I/O
		flen_t read(char *buf, flen_t size, flen_t offset);
		flen_t write(const char *buf, flen_t size, flen_t offset);
		int truncate(flen_t offset);
		int unlink();
		void release();

		//char *get_cache_path(char *buffer);
	private:
		//void offset2blk_offset(flen_t offset, int *blk_index, flen_t* blk_offset);
};

class DfsDir
{
	public:
		int max_file;
		DfsFile **files;

		DfsDir(int max_file);
		~DfsDir();

		DfsFile *find(const char *filename);
		DfsFile *add(DfsFile *file);
		int save(const char *filename);
		int load(const char *filename);
		int unlink(const char *filename);

		// for debugging
		void list();
};

int upload_file(const char *path, const char *password,
		const char **server, int n_server, int k);
int download_file(const char *filename, int down_len, const char *password,
		const char **server, int n_server, int k);

int upload_file(DfsFile *file);
int download_file(DfsFile *file);

extern volatile bool uploaded;
#endif
