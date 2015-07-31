/*
 * dev_cmd.h
 *
 *  Created on: 2015. 7. 20.
 *      Author: gang
 */

#ifndef DEV_CMD_H_
#define DEV_CMD_H_

#include "dfs_file.h"

int handle_command(char *buf, size_t size, off_t offset);
int read_list(char *buf, size_t size, off_t offset);

bool is_file_exist(DfsFile *file);

extern int dev_size; // the intial file size of the special file 'dev'
extern int my_blocksize;
extern const char *my_password;
extern const char*my_server[];
extern int my_n;
extern int my_k;
extern DfsDir dfsDir;

#endif /* DEV_CMD_H_ */
