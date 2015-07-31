#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <sys/time.h>

#ifdef DEBUG
#define DMSG(...) do { \
			struct timeval tv; \
			gettimeofday(&tv, NULL); \
			fprintf(stderr, "%ld.%ld >>> ", (long)(tv.tv_sec % 100), (long)(tv.tv_usec / 1000)); \
			fputs(__PRETTY_FUNCTION__, stderr); \
	        fprintf(stderr, ": " __VA_ARGS__); \
			fputc('\n', stderr); } while(0)
#else
#define DMSG(...) do {} while(0)
#endif

#define STAMP(...) do { \
			fprintf(stdout, "\n%s >>> ", stamp_time()); \
			fprintf(stdout, __VA_ARGS__); \
			fputc('\n', stdout); \
			fflush(stdout); } while(0)

char *stamp_time();

#endif
