#ifndef _POSIX_IF_H
#define _POSIX_IF_H

#include <sys/socket.h>

#define IFNAMSIZ		16

#define IFF_UP			0x01
#define IFF_BROADCAST	0x02
#define IFF_LOOPBACK	0x04

struct ifreq
{
	char ifr_name[IFNAMSIZ];
	struct sockaddr ifr_addr,ifr_dstaddr,ifr_broadaddr;
	int ifr_flags,ifr_index,ifr_metric,ifr_mtu,ifr_phys,ifr_media;
	void* ifru_data;
};

struct ifconf
{
	int ifc_len;
	char* ifc_buf;
	struct ifreq* ifc_req;
};

#endif