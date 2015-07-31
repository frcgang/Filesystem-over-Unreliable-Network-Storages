//#include <string>
#include <stdio.h>
#include "rs_config.h"

RsConfig::RsConfig(int *mesg_capacity, int mesg_server_count,
		int *parity_capacity, int parity_server_count, const char *prefix)
{
	int block_index = 0;
	int max_capacity = 0;
	mesg_count = mesg_server_count;
	parity_count = parity_server_count;
	server = new RsServerInfo[mesg_server_count + parity_server_count];
	for(int i = 0; i < mesg_server_count; i++)
	{
		server[i].set(block_index, mesg_capacity[i]);
		block_index += mesg_capacity[i];
		if(mesg_capacity[i] > max_capacity)
			max_capacity = mesg_capacity[i];
	}
	k = block_index;

	for(int i = 0; i < parity_server_count; i++)
	{
		server[i+mesg_server_count].set(block_index, parity_capacity[i]);
		block_index += parity_capacity[i];
		if(mesg_capacity[i] > max_capacity)
			max_capacity = mesg_capacity[i];
	}
	n = block_index;

	filename_prefix = prefix;

	// test if valid config
	if((n - max_capacity) < k)
		throw "Invalid capacity configuration";
}


RsConfig::~RsConfig()
{
	if(server) delete[] server;
}

char *RsConfig::get_filename(int index)
{
	const char *type = (index < mesg_count) ? "_m" : "_p";
	sprintf(tmp_filename, "%s%s%d.df", filename_prefix, type, index);
	return tmp_filename;
}
