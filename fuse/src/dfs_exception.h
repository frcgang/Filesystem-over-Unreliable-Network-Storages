#ifndef _DFS_EXCEPTION_H
#define _DFS_EXCEPTION_H

#include "debug.h"

class DfsException
{
	public:
		const char *msg;

		DfsException(const char *m) {
			DMSG("%s" , m);
			msg = m; 
		}
};

#endif
