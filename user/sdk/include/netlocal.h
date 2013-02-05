#ifndef _NET_LOCAL_H
#define _NET_LOCAL_H

#define PF_LOCAL	0

struct LocalSocketAddr
{
	unsigned long int family;
	char name[20];
};

#endif
