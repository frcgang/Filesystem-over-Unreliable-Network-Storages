#include "debug.h"

struct timeval last_tv = { 0, 0};

char *stamp_time()
{
	static char buf[30];
	struct timeval now;
	gettimeofday(&now, NULL);
	long diff = (long)(now.tv_sec - last_tv.tv_sec) * 1000000
		+ (long)now.tv_usec - (long)last_tv.tv_usec;

	sprintf(buf, "[STAMP] %ld.%03ld (+%ld.%06ld)",
			(long)(now.tv_sec % 100), (long)(now.tv_usec / 1000),
			diff/1000000, diff % 1000000);

	last_tv = now;
	return buf;
}
