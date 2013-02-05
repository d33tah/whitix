#ifndef LINUX_MODULE_H
#define LINUX_MODULE_H

#include <llist.h>

#define MODULE_NAME_LEN	(64 - sizeof(unsigned long))

#define quickcall __attribute__ ((regparm(3)))

enum module_state
{
	MODULE_STATE_LIVE,
	MODULE_STATE_COMING,
	MODULE_STATE_GOING,
};

struct module
{
	enum module_state state;
	struct ListHead list;
	char name[MODULE_NAME_LEN];
};

#endif
