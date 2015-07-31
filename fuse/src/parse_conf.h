#ifndef _PARSE_CONF_H
#define _PARSE_CONF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class KeyValue
{
public:
	char *key;
	char *value;

	KeyValue(const char *key, const char *value)
	{ this->key = strdup(key); this->value = strdup(value); }

	~KeyValue() { free(key); free(value); }

	//char *get_value() { return value; }
	//char *get_int_value() { return atoi(str_value); }
	char *to_string(); 
};

class DfsConf
{
	static const int max_kv_size = 100;
public:
	KeyValue *kv[max_kv_size];
	int num_conf;

	DfsConf(const char *filename);
	~DfsConf();
	int add(const char *key, const char *value);
	const char *get(const char *key, int index = 0);
	int get_int(const char *key, int index = 0);
	int count(const char *key);
};

#endif
