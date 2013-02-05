#ifndef NETDB_H
#define NETDB_H

struct hostent
{
	char* h_name;
	char** h_aliases;
	int h_addrtype;
	int h_length;
	char** h_addr_list;
};

struct protoent
{
	char* p_name;
	char** p_aliases;
	int p_proto;
};

extern int h_errno;

#define HOST_NOT_FOUND	0x01
#define NO_DATA			0x02
#define NO_RECOVERY		0x04
#define TRY_AGAIN		0x08
#define NO_ADDRESS		0x10

#define PF_UNIX		0x01

#endif
