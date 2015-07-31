#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include "download_buffer.h"
#include "dfs_exception.h"

#define DB_DEBUG

int DownloadBuffer::buffer_size = 1024 * 64; // 64KB

DownloadBuffer::DownloadBuffer()
{
	buffer = new unsigned char [buffer_size];
	read_ptr = buffer;
	available = 0;
	status = NONE;
	fd = -1;
	is_first_read = true;
}

DownloadBuffer::~DownloadBuffer()
{
	close_buffer(); // just pclose
	if(buffer) delete[] buffer;
	buffer = NULL;
}

int DownloadBuffer::open_command(char *command)
{
	this->command = command;
	storage = popen(command, "r");
	if(storage == NULL) throw new DfsException("cannot popen");

	fd = fileno(storage);

	// do not check here; check when merge
	/*
	// check header
	char tmp[magic_size];
	int r = read(fd, tmp, magic_size);
	if(r != magic_size)
	{
		DMSG("cannot read magic : %s", command);
		pclose(storage);
		storage = NULL;
		throw new DfsException("read magic");
	}
	if(memcmp(tmp, magic, magic_size) !=0)
	{
		DMSG("invalid magic %02X %02X.. : %s", tmp[0], tmp[1], command);
		pclose(storage);
		storage = NULL;
		throw new DfsException("different magic");
	}
	*/

	read_total = 0;
	status = OPEN;
	return fd;
}

// returns the return value of pclose()
int DownloadBuffer::close_buffer()
{
	int exit_code = 0;
	if(is_open())
	{
		//DMSG("\n");
		//exit_code = pclose(storage);
		async_pclose(storage);
		if(exit_code != 0)
		{
			status = DROPPED; // TODO unreachable
			//fprintf(stderr, "(%d) ", exit_code);
			//perror(command);
			//throw "download_buffer.cpp: pclose error";
		}
		else
		{
			status = FINISHED;
			//fprintf(stderr, "pclose succeeded %s\n",command);
		}
	}
	return exit_code;
}

void DownloadBuffer::drop()
{
	if(is_open()) async_pclose(storage);
	status = DROPPED;
}

unsigned char *DownloadBuffer::write_ptr()
{
	int r_index = read_ptr - buffer;
	int w_index = (r_index + available) % buffer_size;
	return buffer+w_index;
}

int DownloadBuffer::write_buffer(unsigned char *data, int size)
{
	if(size > get_empty()) return -1;

	unsigned char *wptr = write_ptr();
	int empty_right = buffer + buffer_size - wptr;

	if((read_ptr > wptr) || (empty_right >= size))
	{
		memcpy(wptr, data, size);
	}
	else
	{
		memcpy(wptr, data, empty_right);
		memcpy(buffer, data + empty_right, size-empty_right);
		/*
		fprintf(stderr, "writing [%d,%d) [%d,%d)\n",
				wptr-buffer, wptr-buffer+empty_right,
				0, size-empty_right);
		*/
	}

	available += size;

	return 0;
}

int DownloadBuffer::proceed(int processed_total)
{
	bool not_eof = false;
	if(!is_open()) return -2; //error

	if(is_first_read)
	{
		is_first_read = false;
		char buf[magic_size];
		int r = read(fd, buf, magic_size);
		if(r != magic_size)
		{
			DMSG("cannot read magic");
			return -3;
		}
		if(memcmp(buf, magic, magic_size) != 0)
		{
			DMSG("magic differs");
			return -3;
		}
	}

	// follow up, if too late
	if((read_total + available) < processed_total)
	{
		consume(); // read_total += available; available = 0
		while(read_total  < processed_total)
		{
			long to_skip = processed_total - read_total;
			long to_read = (to_skip > SSIZE_MAX) ? SSIZE_MAX : to_skip;
			char skip_buf[to_read];
			int r = read(fd, skip_buf, to_read);
//fprintf(stderr, "Downloadbuffer::proceed reads %d bytes\n", r);//TEMP
			read_total += r;
			if(r < to_read) break;
		}
#ifdef DEBUG
		fprintf(stderr, "followed up to %ld\n\t\t", read_total);
#endif
		not_eof = true;
		//if(read_total < processed_total)
		//	return 0; // no more to read in the pipe
	}

	int empty = get_empty();
	if(empty == 0) return 0; // no buffer

	unsigned char tmpbuf[empty];
	int r = read(fd, tmpbuf, empty);
	if(r > 0)
	{
		write_buffer(tmpbuf, r);
		return r;
	}
	else if(r==0) // EOF
	{
		if(not_eof)
		{
			DMSG("followed up");
			return 0; // followed up
		}
		else
		{
			DMSG("EOF");
			return -1;
		}
	}
	else
	{
		perror("DownloadBuffer::proceed");
		if((errno == EAGAIN) || (errno== EINTR)) return 0; // TODO cause hanging?
		return -2; // error
	}

}

int DownloadBuffer::read_buffer(unsigned char *buf, int size)
{
	int to_read = (size > available) ? available : size;

	//unsigned char *ret = read_ptr;
	int right_avail = buffer_size - (read_ptr - buffer);
	if(right_avail >= to_read)
	{
		memcpy(buf, read_ptr, to_read);
	}
	else
	{
		memcpy(buf, read_ptr, right_avail);
		memcpy(buf+right_avail, buffer, to_read-right_avail);
	}

	read_ptr = buffer + (((read_ptr-buffer) + to_read) % buffer_size);
	available -= to_read;
	read_total += to_read;
	return to_read;
}

// dipose of unnecessary data
void DownloadBuffer::consume()
{
	read_ptr = buffer + (((read_ptr-buffer) + available) % buffer_size);
	read_total += available;
	available = 0;
}

//////////////////////////////////////
//
// BufferPool
//
//////////////////////////////////////

BufferPool::BufferPool(int n, int k, int buffer_size)
{
	DownloadBuffer::buffer_size = buffer_size;
	buffer = new DownloadBuffer[n];
	this->n = n;
	this->k = k;
	this->open_index = 0;
	this->processed_total = 0;
}

BufferPool::~BufferPool()
{
	delete[] buffer;
}

// get minimum empty byte size among active buffers;
int min_empty(DownloadBuffer *buffer, int n)
{
	int i;
	int min = DownloadBuffer::buffer_size;
	for(i = 0; i < n; i++)
	{
		if(buffer[i].is_open())
		{
			int empty = buffer[i].get_empty();
			if(empty < min) min = empty;
		}
	}
	return min;
}

bool BufferPool::is_any_full()
{
	int i;
	for(i = 0; i < n; i++)
	{
		if(buffer[i].is_full()) return true;
	}
	return false;
}

bool BufferPool::is_any_open()
{
	int i;
	for(i = 0; i < n; i++)
	{
		if(buffer[i].is_open()) return true;
	}
	return false;
}

int BufferPool::get_finished_count()
{
	int i;
	int count = 0;
	for(i=0; i<n; i++)
		if(buffer[i].get_status() == DownloadBuffer::FINISHED) count++;
	return count;
}

int BufferPool::get_closed_count()
{
	int i;
	int count = 0;
	for(i=0; i<n; i++)
	{
		DownloadBuffer::Status s = buffer[i].get_status();
		if((s == DownloadBuffer::FINISHED) || (s == DownloadBuffer::DROPPED))
				count++;
	}
	return count;
}

int BufferPool::open_command(char *cmd) // returns fd
{
	if(open_index >= n) return -1; // open overflow
	return buffer[open_index++].open_command(cmd);
}

int BufferPool::get_fds(fd_set *fds) // return max fd
{
	int i;
	int fd;
	int max_fd = 0;
	FD_ZERO(fds);
	for(i = 0; i < n; i++)
	{
		if(buffer[i].is_open())
		{
			if(buffer[i].get_empty() > 0) // if buffer full, cannot read more
			{
				fd = buffer[i].get_fd();
				FD_SET(fd, fds);
				if(fd > max_fd) max_fd = fd;
			}
			else DMSG("skip %d",i);
		}
	}
	return max_fd;
}

void BufferPool::download(fd_set *fds)
{
	int i;
	int r; 
	for(i = 0; i < n; i++)
	{
		if(buffer[i].is_open() && FD_ISSET(buffer[i].get_fd(), fds))
		{
			r = buffer[i].proceed(processed_total);
			if(r == -1) // EOF
			{
				/*
				int exit_code = buffer[i].close_buffer();
				if(exit_code)
					//perror("error in close_buffer");
					fprintf(stderr, "\ncannot get the stream of %s\n",
							buffer[i].get_command());
				*/
				int total_i = buffer[i].get_downloaded();
				bool good_eof = true;
				for(int j=0; j < n; j++)
				{
					if(buffer[j].get_downloaded() > total_i)
						good_eof = false;
				}
				if(good_eof)
					buffer[i].close_buffer();
				else r = -2; // too short
 			}

			if(r < -1)
			{
				DMSG("DROP %d", i);
				buffer[i].drop();
				//throw "proceed error of ";// + i;
			}
#ifdef DEBUG
			else // TEMP
			{
//				fprintf(stderr, "\nread %d bytes at %d", r, i);
			}
#endif
		}
	}
}

static int compare_downloaded(const void *buf1, const void *buf2)
{
	DownloadBuffer * const *b1 = (DownloadBuffer * const *)buf1;
	DownloadBuffer * const *b2 = (DownloadBuffer * const *)buf2;

	return ((*b2)->get_downloaded() - (*b1)->get_downloaded()); // descending order
}

int BufferPool::readable_count()
{
	int i;
	//int open_count = 0;
	DownloadBuffer *b[n];

	//for(i = 0; i < n; i++) if(buffer[i].is_open())
	//	b[open_count++] = &buffer[i];
	for(i = 0; i < n; i++)
		b[i] = &buffer[i];
	qsort(b, n, sizeof(DownloadBuffer *), compare_downloaded);

	//if(open_count < k) throw "broken read";
	int to_read = b[k-1]->get_downloaded() - processed_total;

	/*printf("\n");
for(i=0;i<n;i++)
	printf("  %d/%d/%d",
	b[i]->get_available(),
	b[i]->get_read_total(),
	b[i]->get_downloaded()
	); printf("  %d,%d\n",to_read,processed_total);*/

	return to_read;
}

/*
int BufferPool::read_buffer(int index, unsigned char *buf,  int to_read)
{
	if(!buffer[index].is_open()) return -1;
	return buffer[index].read_buffer(buf, to_read);
}
*/

// buf_arr = (unsigned char *)[n], ->  (unsigned char)[buffer_size]
int BufferPool::read_buffer(unsigned char *buf_arr[], int to_read)
{
	int i;
	//int to_read = readable_count();
	int read_count = 0;
	//int r;
	for(i = 0; i < n; i++)
	{
		if(!buffer[i].is_ever_opened()) // there might be left data after FINSHED
		{
			buf_arr[i] = NULL;
			//DMSG("BufferPool::read_buffer : [%d] not opened\n",i);
			continue;
		}
#ifdef TEMP_DEBUG2
		printf("BufferPool::read_buffer : [%d] read total=%d avail=%d\n",i,
		buffer[i].get_read_total(), buffer[i].get_available());
#endif
		if(buffer[i].get_downloaded() < (processed_total + to_read))
		{
			buffer[i].consume();
			//buffer[i].drop(); printf("closing %d\n",i);
			buf_arr[i] = NULL;
#ifdef TEMP_DEBUG2
		printf("BufferPool::read_buffer : [%d] consumed\n",i);
#endif
			continue;
		}

		read_count++;
		if(buffer[i].read_buffer(buf_arr[i], to_read) != to_read)
			throw "read failure in BufferPool::read_buffer";
#ifdef TEMP_DEBUG2
		printf("BufferPool::read_buffer : [%d] read %d bytes\n",i,  to_read);
#endif
	}
	//if(read_count < k) throw "not enough buffers in BufferPool::read_buffer";

	processed_total += to_read;
	return to_read;
}

void BufferPool::print_status(FILE *err)
{
	int i;
	for(i=0; i<n; i++)
	{
		char s = '\0';
		switch(buffer[i].get_status())
		{
		case DownloadBuffer::OPEN: s ='o'; break;
		case DownloadBuffer::FINISHED: s ='f'; break;
		case DownloadBuffer::DROPPED: s ='d'; break;
		case DownloadBuffer::NONE: s ='n'; break;
		}
		fprintf(err, "%d%c%5d/%6d \t", i,
				s,
				buffer[i].get_available(),
				buffer[i].get_downloaded());
	//fprintf(err, "%ld", processed_total);
	//fprintf(err, "\n");
	}
}

/*
int main(int argc, char ** argv)
{
	DownloadBuffer buffer[10];
	
	buffer[0].write((unsigned char *)"123456780",10);	
	buffer[1].write((unsigned char *)"123456780",1);	
	unsigned char *r = buffer[0].read(2);
	if(r != NULL) printf("%s\n", r);
	r = buffer[0].read(10);
	if(r != NULL) printf("%s\n", r);
	else printf("NULL\n");

}
*/
