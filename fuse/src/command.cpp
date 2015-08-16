#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include "download_buffer.h"
#include "updown_command.h"
#include "dfs_file.h"
#include "debug.h"

//#define BUFFER_SIZE (1024 * 1) 
//extern int debug_level;
//int verbose = 0;

int download(FILE *out, char **cmd, int n, int k, long down_len = 0);
int upload(FILE *in, char **cmd, int n, int k);


const char *openssl = "openssl";
//const char *openssl = "tmp/dfs_echo"; // without encryption
const char *curl = "curl";

int upload_file(const char *path, const char *password, const char **server, int n_server, int k)
{
	// ignore pipe error
//	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
//	    perror("signal");

	UpDownCommand cmd(path, openssl, password,
			curl, server, n_server);
	DMSG("%s:\n\t%s\n\t%s", path, cmd.curl_up_cmd[0], cmd.openssl_up_cmd); //TEMP

	FILE *fp = popen(cmd.openssl_up_cmd, "r");
	int up_size = upload(fp, cmd.curl_up_cmd, n_server, k);
	int ret = pclose(fp);
	if(ret < 0) return ret;
	else return up_size;
}

int download_file(const char *filename, int down_len, const char *password, 
		const char **server, int n_server, int k)
{
	//TEMP
	struct timeval tv1, tv2; \
	gettimeofday(&tv1, NULL);

	UpDownCommand cmd(filename, openssl, password,
			curl, server, n_server);


	DMSG("%s:\n\t%s\n\t%s", filename, cmd.curl_down_cmd[0], cmd.openssl_down_cmd); // TEMP
	FILE *fp = popen(cmd.openssl_down_cmd, "w");
	int r = download(fp, (char **)cmd.curl_down_cmd, n_server, k, down_len);

	DMSG("download returns %d", r);

	// ignore pipe error
//	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
//	    perror("signal");
//
	//int err = ferror(fp);
	if(feof(fp)) DMSG("EOF");
	else DMSG("NOTEOF");
	//int err = pclose(fp);
	async_pclose(fp);
	DMSG("pclose");

	fp = fopen(filename, "r");
	fseek(fp, 0L, SEEK_END);
	int file_len = ftell(fp);
	fclose(fp);
	DMSG("fclose");

	static long tsec = 0;
	static long tusec =0;
	long sec, usec;
	gettimeofday(&tv2, NULL);
	usec = tv2.tv_usec - tv1.tv_usec;
	sec = tv2.tv_sec - tv1.tv_sec;
	if(usec < 0){ sec--; usec += 1000000; }
	tsec += sec; tusec +=usec; if(tusec >= 1000000) { tsec += 1; tusec -= 1000000; }
	DMSG("DOWNLOAD TIME: %ld.%ld (%ld.%ld)", sec, usec, tsec, tusec);

//	if(err < 0)
//		return err;
	return file_len;
}
/*
int upload_file(DfsFile *file)
{
	DMSG("%s", file->filename);
	if(file->cache_fp)
	{
		fclose(file->cache_fp);
		file->cache_fp = NULL;
	}
	char cache_filename[CACHE_DIR_LEN + strlen(file->filename) + 1];
	file->get_cache_path(cache_filename);

	file->enc_size = upload_file(cache_filename, file->password,
			file->server, file->n, file->k);
	if(file->enc_size > 0) file->status = DOWNLOADED;
	return file->enc_size;
}

int download_file(DfsFile *file)
{
	char cache_filename[CACHE_DIR_LEN + strlen(file->filename) + 1];
	file->get_cache_path(cache_filename);

	int ret = download_file(cache_filename, file->enc_size, file->password,
			file->server, file->n, file->k);
	if(ret > 0)
	{
		file->size = ret;
		file->status = DOWNLOADED;
	}
	return ret;
}
*/

// test code
#ifdef TEST_COMMAND
int main(int argc, char **argv)
{
	char *filename = NULL;
	char *password = (char *)"1234";
	int i;

	if(argc > 2) filename = argv[2];
	if(argc > 3) password = argv[3];
	if(argc <= 2)
	{
		fprintf(stderr,
				"Usage: %s {u|d} <filename> [<password> [<file_size>]]\n",
				argv[0]);
		return 1;
	}

	int n = 5;
	int k = 3;
	const char *servers[] = {
		"file:///home/gang/workspace/dfs/storage/",
		"file:///home/gang/workspace/dfs/storage/",
		"file:///home/gang/workspace/dfs/storage/",
		"file:///home/gang/workspace/dfs/storage/",
		"file:///home/gang/workspace/dfs/storage/",
	};

	UpDownCommand cmd(filename, (char *)"openssl", password,
			(char *) "curl", (char **)servers, n);

	if(verbose)
	{
		fprintf(stderr,"ENC : %s\n", cmd.openssl_up_cmd);
		fprintf(stderr,"DEC : %s\n", cmd.openssl_down_cmd);
		for(i = 0; i < n; i++)
		{
			if(verbose)
			{
				fprintf(stderr,"UP %d: %s\n", i, cmd.curl_up_cmd[i]);
				fprintf(stderr,"DN %d: %s\n", i, cmd.curl_down_cmd[i]);
			}
		}
	}


	//FILE *fp;
	int down_len = 0;

	clock_t start_time = clock();
	int exit_code = 0;
	try
	{
		if(argc>1)
		{
			switch(argv[1][0])
			{
				case 'u':
					exit_code = upload_file(filename, password,
							(char **)servers, n, k);
					break;
				case 'd':
					if(argc > 4) down_len = atoi(argv[4]);
					exit_code = download_file(filename, down_len, password,
						(char **)servers, n, k);
					break;
				default:
					throw "undefined command";
			}
			//int exit_code = pclose(fp);
			if(exit_code < 0) throw "enc/dec error";
			//else fprintf(stderr, "enc/dec exits with %d.\n", exit_code);
		}
	}
	catch(char const* msg)
	{
		fprintf(stderr, "error exit: %s\n", msg);
	}

	fprintf(stderr, "It took %f seconds. (exit_code = %d)\n",
			(clock() - start_time) / (float)CLOCKS_PER_SEC,
			exit_code);

}
#endif
