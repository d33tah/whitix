#ifndef IN_H
#define IN_H

struct in_addr
{
	union
	{
		struct {
			unsigned char s_b1,s_b2,s_b3,s_b4;
		}S_un_b;
		struct {
			unsigned short s_w1,s_w2;
		}S_un_w;
		unsigned long s_addr;
	};
};

struct sockaddr_in
{
	short sin_family;
	unsigned short sin_port;
	struct in_addr sin_addr;
	char sin_zero[8];
};

#define IP_OPTIONS		0x001
#define IP_PKTINFO		0x002
#define IP_MULTICAST	0x004
#define IP_MULTICAST_TTL	0x008
#define IP_MULTICAST_LOOP	0x010
#define IP_ADD_MEMBERSHIP	0x020
#define IP_DROP_MEMBERSHIP	0x040
#define IP_MULTICAST_IF		0x080

#define INADDR_LOOPBACK		0x7F000001

struct ip_mreqn
{
	struct in_addr imr_multiaddr;
	struct in_addr imr_address;
	int imr_ifindex;
};

typedef unsigned long in_addr_t;

#endif
