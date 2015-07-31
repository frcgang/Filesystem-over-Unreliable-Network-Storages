#include <stdio.h>
#include <pthread.h>
#include "distribute.h"
#include "debug.h"

static Distribute *dfs = NULL;
static pthread_mutex_t dfs_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool is_simple(unsigned char **split, int k)
{
	int i;
	for(i = 0; i < k; i++)
	{
		if(split[i] == NULL) return false;
	}
	return true;
}

void write_out(FILE *out, unsigned char *buf, int size)
{
	int to_write = size;
	int written;
	char *wptr = (char *)buf;

	while(to_write)
	{
		written = fwrite(wptr, sizeof(unsigned char), to_write, out);
		if(written <= 0)
		{
			if(feof(out) || ferror(out))
				throw "cannot write the merged data";
		}

		wptr += written;
		to_write -= written;
	}
}

int read_in(FILE *in, unsigned char *buf, int size)
{
	int to_read = size;
	int read;
	char *rptr = (char *)buf;

	while(to_read)
	{
		read = fread(rptr, sizeof(unsigned char), to_read, in);
		if(read <= 0)
		{
			if(feof(in)) return (size - to_read); // EOF
			if(ferror(in)) throw "cannot read the date";
		}
		rptr += read;
		to_read -= read;
	}
	return size;
}


void merge(unsigned char **split, FILE *out, int n, int k, int merge_size)
{
	int i;

#ifdef TEMP_DEBUG2

	//printf("merge: n=%d k=%d ms=%d\n", n,k,merge_size);
	printf("\n");
	for(i=0; i<n; i++){
		if(split[i] ==0)
			printf("split[%d] is null\n", i);
		else printf("split[%d]=%02X%02X..\n", i,split[i][0],split[i][1]);
	}
#endif

	if(merge_size == 0) return;

	if(is_simple(split, k))
	{
		// do simple merge
#ifdef DEBUG
		fprintf(stderr, "smpl:");//TEMP
#endif
		for(i = 0; i < merge_size; i++)
		{
			int split_select = i % k;
			int split_index = i / k;

			if(fputc(split[split_select][split_index], out) == EOF)
			{ // on error
				throw "merge wirte error";
			}
		}
		return;
	}

	//////// CRITICAL REGION : BEGIN
	// As 'dfs' is static, it should be locked
//	int fd_value = fileno(out);
//	DMSG("(%d) WAIT LOCK", fd_value);
	pthread_mutex_lock(&dfs_mutex);
//	DMSG("(%d) LOCK OWNED", fd_value);

	// reed-solomon decoding
#ifdef DEBUG
	fprintf(stderr, "rs  :");//TEMP
#endif
	if(dfs == NULL)
	{
		dfs = new Distribute(n,k);
	}
	else
	{
		dfs->reset(n, k);
	}

	int mlen;
	unsigned char *decoded = dfs->decode_merge(split, (merge_size-1) / k + 1, mlen);
	try
	{
		write_out(out, decoded, merge_size);
	}
	catch(const char *emsg)
	{
		delete[] decoded;
		throw emsg;
	}

	delete[] decoded;

//	DMSG("(%d) UNLOCK", fd_value);
	pthread_mutex_unlock(&dfs_mutex);
	//////// CRITICAL REGION : END

	if( (mlen/k) != ((merge_size +k -1) /k) )
		throw "merge length error";
}


int split(FILE *in, FILE**out, int n, int k)
{
	int i;
	int r;
	int buffer_size = 1024 * k;
	unsigned char read_buffer[buffer_size];
	int count = 0;


	if(dfs == NULL)
	{
		dfs = new Distribute(n,k);
	}
	else
	{
		dfs->reset(n,k);
	}

	while((r = read_in(in, read_buffer, buffer_size)) > 0)
	{
		int enc_len;
		if(r == 0) break;
		unsigned char **encoded = dfs->encode_split(read_buffer, r, enc_len);

		try
		{
			for(i = 0; i < n; i++)
			{
				write_out(out[i], encoded[i], enc_len);
			}
		}
		catch (const char *emsg)
		{
			dfs->deleteEncoded(encoded);
			throw emsg;
		}

		dfs->deleteEncoded(encoded);

		count += r;
		if(r < buffer_size) break;
	}

	return count;
}
