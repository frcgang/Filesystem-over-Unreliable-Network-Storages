#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <strings.h>
#include "download_buffer.h"
#include "dfs_exception.h"

//extern int verbose;

const char *magic = "MAGIC";
const int magic_size = 5;

int split(FILE *in, FILE**out, int n, int k);

int make_split_files(FILE *in, FILE **out, int n, int k)
{
	int i;
	//FILE *out[n];
	char tmp_split_name[] = "split-000.tmp";


	for(i = 0; i < n; i++)
	{
		tmp_split_name[6] = '0' + (i / 100);
		tmp_split_name[7] = '0' + (i / 10 % 10);
		tmp_split_name[8] = '0' + (i % 10);
		out[i] = fopen(tmp_split_name, "w");
		//fprintf(stderr, "tmp name %s\n", tmp_split_name);
		if(out[i] == NULL) throw "cannot open split files";
	}
	int in_size = split(in, out, n, k);

	// rewind for reading
	// reopen to read
	for(i = 0; i < n; i++)
	{
		fclose(out[i]);

		tmp_split_name[6] = '0' + (i / 100);
		tmp_split_name[7] = '0' + (i / 10 % 10);
		tmp_split_name[8] = '0' + (i % 10);

		out[i] = fopen(tmp_split_name, "r");
		//fprintf(stderr, "tmp name %s\n", tmp_split_name);
		if(out[i] == NULL) throw "cannot open split files";
		//rewind(out[i]);
	}

	return in_size;
}

static long get_max(long *count, int n)
{
	long max = 0;

	for(int i=0; i<n; i++)
	{
		if(count[i] > max) max = count[i];
	}
	return max;
}

// returns 'in' size
int upload(FILE *in, char **cmd, int n, int k)
{
	int i;
	int error_count = 0;
	int complete_count = 0;

	FILE *split_file[n];
	FILE *storage[n];
	int storage_fd[n];
	long write_count[n];
	bzero(write_count, sizeof(long) * n);

	int in_size = make_split_files(in, split_file, n, k);

	for(i = 0; i < n; i++)
	{
		storage[i] = popen(cmd[i], "w");

		if(storage[i] != NULL)
		{
			storage_fd[i] = fileno(storage[i]);

			// write header (MAGIC)
			int w = write(storage_fd[i], magic, magic_size);
			if(w != magic_size)
			{
				DMSG("FAILED TO WRITE MAGIC [%d]", i);
				pclose(storage[i]);
				storage[i] = NULL;
			}

		}

		if(storage[i] == NULL)
		{
			error_count++;
			fclose(split_file[i]);
		}
		else
		{
			int flags = fcntl(storage_fd[i], F_GETFL, 0);
			fcntl(storage_fd[i], F_SETFL, flags | O_NONBLOCK); // set non blocking write
		}
	}
	if(error_count > (n-k)) throw DfsException("too many storage failures");

	int buffer_size = 10240;
	//int buffer_size = 1024;
	char buffer[buffer_size];
	fd_set fds;

	while(complete_count < (n-error_count))
	{
		// set fds
		int maxfd = -1;
		FD_ZERO(&fds);
		for(i = 0; i < n; i++)
		{
			if(storage[i])
			{
				int fd = storage_fd[i];
				if(fd > maxfd) maxfd = fd;
				FD_SET(fd, &fds);
			}
		}

		int result = select(maxfd+1, NULL, &fds, NULL, NULL);
		if( result < 0)
		{
			throw "error in select/upload";
		}

		for(i = 0; i < n; i++)
		{
			if(storage[i] && FD_ISSET(storage_fd[i], &fds))
			{
				int r = fread(buffer, sizeof(char), buffer_size, split_file[i]);
				int w;
				if(r > 0)
				{
					w = write(storage_fd[i], buffer, r); // non blocking write
					write_count[i] += w;
					//fprintf(stderr, "writing %d - %d/%d\n", i, w, r);
					if(w < r){
						//fprintf(stderr, "[%d] gets back %d\n", i, (w-r));
						fseek(split_file[i], w-r, SEEK_CUR); // get back
					}
				}
				else if(feof(split_file[i]))
				{
					//if(verbose) fprintf(stderr, "closing %d - (%d)\n", i, r);
					fclose(split_file[i]);
					async_pclose(storage[i]); //TODO
					//pclose(storage[i]); // synchronous close for safe writing
					storage[i] = NULL; // set the storage is closed
					if(write_count[i] >= get_max(write_count, n))
						complete_count++;
					else DMSG("early feof");
				}
			}
		} // for

#ifdef DEBUG
		// status
		for(i = 0; i < n; i++)
		{
			if(storage[i])
			{
				fprintf(stderr, " %6ld ", write_count[i]);
			}
			else
			{
				fprintf(stderr, "     -0 ");
			}
		}
		fprintf(stderr, "\n");
#endif
	}// while 

	return in_size;
}

