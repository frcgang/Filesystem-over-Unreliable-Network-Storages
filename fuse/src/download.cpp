#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <strings.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include "download_buffer.h"

#define BUFFER_SIZE (1024 * 8) 
int debug_level = 0;

void merge(unsigned char **split, FILE *out, int n, int k, int merge_size);
//void split(FILE *in, FILE**out, int n, int k);

void print_binary(FILE *out, unsigned int b)
{
	int i;
	unsigned int index = 0x1;
	for(i = 0; i < 8; i++)
	{
		if((index << i) & b) fprintf(out, "V");
		else fprintf(out, "_");
	}
}

int download(FILE *out, char **cmd, int n, int k, long down_len)
{
	BufferPool buffer_pool(n, k, BUFFER_SIZE);
	//DownloadBuffer buffer[num];

	int i;
	fd_set fds;
	//int flags;
	int maxfd = -1;
	int result;
	//int r; // read bytes
	long down_left = (down_len > 0) ? down_len : LONG_MAX;
	//int close_count = 0;
	//fprintf(stderr, "to download %d\n", down_left);

	for(i = 0; i < n; i++)
	{
		int fd = buffer_pool.open_command(cmd[i]);
		int flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK); // non blocking
		//if(fd>maxfd) maxfd = fd;
	}

	//clock_t start, sstart, elapsed;
	//start = clock();
	while((buffer_pool.get_finished_count() < k) && (buffer_pool.get_closed_count() < n))
	{
		maxfd = buffer_pool.get_fds(&fds);

		//sstart = clock();
		result = select(maxfd+1, &fds, NULL, NULL, NULL);
		//elapsed = clock() - sstart;
		//if(elapsed > 1000)
		//{
		//	if(debug_level) printf("waited to read for %f.\n", elapsed / (float)CLOCKS_PER_SEC);
		//}

		if(result == -1)
		{
			perror("error in select");
			return -1;
		}
		else
		{
#ifdef DEBUG
			print_binary(stderr, (unsigned int)fds.fds_bits[0]);
			fprintf(stderr, " \t");
#endif
			buffer_pool.download(&fds);
			int readable = buffer_pool.readable_count();
#ifdef DEBUG
			fprintf(stderr, ">"); buffer_pool.print_status(stderr);
#endif
			if(readable)
			{
				int i;

				unsigned char *b[n];
				unsigned char buf[n][readable];
				for(i=0; i<n; i++) b[i] = &buf[i][0];

				buffer_pool.read_buffer(b, readable);
				long to_down = readable * k;
				if(to_down > down_left) to_down = down_left;

				// process received data
				//usleep(10000);
				merge(b, out, n, k, to_down);
				down_left -= to_down;
				if(down_left <= 0) break;


				//if(buffer_pool.is_any_full())
				//{
				// drop a slow connection.
				//printf("need to drop.\n");
				//}
			}
			//fprintf(stderr, "  \t "); buffer_pool.print_status(stderr);
#ifdef DEBUG
			fprintf(stderr, " %d\n", buffer_pool.get_processed_total());
#endif

		}
	}

	return 0;
}

// test code
/*
int main(int argc, char **argv)
{
	if(argc > 1) debug_level = atoi(argv[1]);

	const char *cmd[] = {
		"curl -s file:///home/guest/split-00.split",
		"curl -s file:///home/guest/split-01.split",
		"curl -s file:///home/guest/split-02.split",
	};


	try
	{
		download((char **)cmd, 3, 2);
	}
	catch(char const* msg)
	{
		fprintf(stderr, "error exit: %s\n", msg);
	}
}
*/
