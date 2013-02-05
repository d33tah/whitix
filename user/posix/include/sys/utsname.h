#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

struct utsname
{
	char sysname[16];
	char nodename[16];
	char release[10];
	char version[10];
	char machine[16];
};

int uname(struct utsname* name);

#endif
