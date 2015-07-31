#ifndef _DOWNLOAD_BUFFER_H
#define _DOWNLOAD_BUFFER_H

#include <stdio.h>
#include <sys/select.h>

#include "debug.h"

class DownloadBuffer
{
	public:
		enum Status { NONE, OPEN, DROPPED, FINISHED, };

		static int buffer_size;


	private:
		unsigned char *buffer;
		unsigned char *read_ptr; // cursor of reading block
		volatile int available; // number of blocks available
		bool is_first_read; // for magic test

		char *command = NULL;
		FILE *storage = NULL;
		int fd; // file descriptor
		volatile long read_total;
		Status status; //bool

		// private getters
		unsigned char *write_ptr();

	public:
		DownloadBuffer();
		~DownloadBuffer();

		int open_command(char *command);
		int close_buffer();
		void drop();
		int proceed(int processed_total); // receive
		int read_buffer(unsigned char *buf, int size);
		void consume();

		// setters, getters
		int get_available(){ return available; }
		int get_empty(){ return (buffer_size-available); }
		int get_fd(){ return fd; }
		int get_read_total(){ return read_total; }
		int get_downloaded(){ return read_total + available; }
		char *get_command(){ return command; }
		bool is_open(){ return (status == OPEN); }
		bool is_ever_opened(){ return ((status == OPEN) || (status==FINISHED)); }
		Status get_status(){ return status; }
		bool is_full(){ return (is_open() && (available==buffer_size)); }

	private:
		int write_buffer(unsigned char *data, int size);
};

class BufferPool
{
	private:
		DownloadBuffer *buffer;
		int n, k; // reed-solomon (n,k)
		int open_index;
		long processed_total;

	public:
		BufferPool(int n, int k, int buffer_size);
		~BufferPool();

		bool is_any_full();
		bool is_any_open();
		int get_finished_count();
		int get_closed_count();
		int open_command(char *cmd);
		int get_fds(fd_set *fds);
		int get_processed_total(){ return processed_total; }
		//DownloadBuffer *get_buffer(int *n){ return buffer; if(n) *n = this->n; }
		DownloadBuffer *get_buffer(){ return buffer; }
		DownloadBuffer *get_buffer(int index){ return &buffer[index]; }
		int readable_count();
		//int read_buffer(int index, unsigned char *buf, int to_read);
		int read_buffer(unsigned char **buf_arr, int to_read);
		void download(fd_set *fds);
		void print_status(FILE *err);

	private:
};

// pclose without block
void async_pclose(FILE *fp);

extern const char *magic;
extern const int magic_size;
#endif
