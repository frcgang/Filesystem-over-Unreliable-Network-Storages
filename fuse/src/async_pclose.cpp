/*
 * async_pclose.cpp
 *
 *  Created on: 2015. 7. 20.
 *      Author: gang
 */


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "debug.h"
#include "download_buffer.h"
#include "dfs_file.h"

#define MAX_THREAD 128
#define THREAD_SIZE 256
static pthread_t close_thread[THREAD_SIZE];
static pthread_mutex_t close_mutex = PTHREAD_MUTEX_INITIALIZER;;
volatile static int closing_thread_count = 0;
volatile static int thread_index = 0;
volatile bool uploaded = false;

static void *close_it(void *arg)
{
	FILE *fp = (FILE *)arg;
	int r = pclose(fp);
	pthread_mutex_lock(&close_mutex);
	closing_thread_count--;
	if(closing_thread_count <= 0)
	{
		if(uploaded)
		{
			uploaded = false;
			STAMP("all closed %d", closing_thread_count);
		}
		thread_index = 0;
	}
	pthread_mutex_unlock(&close_mutex);
	DMSG("pclose returns %d, close thread count = %d",
			r, closing_thread_count);
	pthread_exit((void *)0);
}

void async_pclose(FILE *fp)
{
	if(fp == NULL) return;
	int error;

	pthread_mutex_lock(&close_mutex);
	{
		if((closing_thread_count < MAX_THREAD)
				&& (thread_index < THREAD_SIZE))
		{
			error = pthread_create(&close_thread[thread_index], NULL, &close_it, (void *)fp);
			if(!error){
				thread_index++;
				closing_thread_count++;
			}
		}
		else error = 1;
	}
	pthread_mutex_unlock(&close_mutex);

	if(error) pclose(fp); // too many threads
}

