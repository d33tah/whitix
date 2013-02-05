#ifndef _POSIX_GRP_H
#define _POSIX_GRP_H

#include <sys/types.h>

struct group
{
	char* gr_name;
	char* gr_passwd;
	gid_t gr_gid;
	char** gr_mem;
};

#endif
