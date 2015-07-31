#include <ctype.h>
#include "parse_conf.h"
#include "dfs_exception.h"
#include "debug.h"

static char *chop(char *str)
{
	char *first = str;
	char *last = str + strlen(str) - 1;

	// remove head
	while(isspace(*first)) first++;

	// remove tail
	while(isspace(*last)) *(last--) = '\0';

	return first;
}

char *KeyValue::to_string()
{
	static char str[100];
	sprintf(str, "'%s'='%s'", key, value);
	return str;
}

DfsConf::DfsConf(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) throw new DfsException("cannot find conf file");
	char line[1024];
	num_conf = 0;

	while(fgets(line, 1024, fp))
	{
		char *key = line;
		if(key[0] == '#') continue;
		char *value = strchr(line, '=');
		if(!value) continue;
		*(value++) = '\0';
		key = chop(key);
		value = chop(value);

		add(key, value);
	}
}

DfsConf::~DfsConf()
{
	for(int i = 0; i < num_conf; i++)
	{
		delete kv[i];
	}
}

int DfsConf::add(const char *key, const char *value)
{
	//DMSG("'%s'='%s'", key, value);
	if(num_conf < max_kv_size)
	{
		kv[num_conf++] = new KeyValue(key, value);
		//DMSG("%s", kv[num_conf-1]->to_string());
		return num_conf;
	}
	else return -1;
}

const char *DfsConf::get(const char *key, int index)
{
	int count = 0;
	for(int i = 0; i < max_kv_size; i++)
	{
		if(strcmp(key, kv[i]->key) == 0)
		{
			if(count == index) return kv[i]->value;
			else count++;
		}
	}
	DMSG("cannot find %s[%d]", key, index);
	return NULL;
}

int DfsConf::get_int(const char *key, int index)
{
	const char *value = get(key, index);
	if(value) return atoi(value);
	else
	{
		DMSG("cannot find %s[%d]", key, index);
		return 0;
	}
}

int DfsConf::count(const char *key)
{
	//DMSG("%s", key);
	int ret = 0;
	for(int i = 0; i < num_conf; i++)
	{
		//DMSG("%s %d %s", key, i, kv[i]->key);
		if(strcmp(key, kv[i]->key) == 0)
		{
			ret++;
		}
	}
	DMSG("%d", ret);
	return ret;
}

#ifdef TEST_CONF
int main(int argc, char **argv)
{
	DfsConf conf("dfs.conf");

	int block_size = conf.get_int("blocksize");
	int n = conf.count("server");
	int k = n/2 +1;

	printf("(n/k) = %d/%d, block size = %d, password = \"%s\"\n",
		       	n, k, block_size, conf.get("password"));

	const char *server[n];
	for(int i = 0; i<n; i++)
	{
		server[i] = conf.get("server", i);
		printf("server %d : \"%s\"\n", i, server[i]);
	}

	return 0;
}
#endif
