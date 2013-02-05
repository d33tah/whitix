#ifndef _POSIX_SOCKET_H
#define _POSIX_SOCKET_H

#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#define AF_UNIX			0x00

/* Not yet supported. */
#define AF_INET			0x01
#define AF_SNA			0x02
#define AF_DECnet		0x03
#define AF_APPLETALK	0x04

#define AF_UNSPEC		0x05

typedef int socklen_t;
typedef unsigned long sa_family_t;

struct sockaddr
{
	sa_family_t sa_family;
	short sa_data[14];
};

struct linger
{
	int l_onoff;
	int l_linger;
};

#define SO_DEBUG		0x1
#define SO_REUSEADDR	0x2
#define SO_KEEPALIVE	0x4
#define SO_DONTROUTE	0x8
#define SO_LINGER		0x10
#define SO_BROADCAST	0x20
#define SO_OOBINLINE	0x40
#define SO_SNDBUF		0x80
#define SO_RCVTIMEO		0x100
#define SO_SNDTIMEO		0x200
#define SO_ERROR		0x400
#define SO_RCVBUF		0x800
#define SO_SNDLOWAT		0x1000
#define SO_RCVLOWAT		0x2000
#define SO_TYPE			0x4000

/* CHANGE */
#define SOL_SOCKET		0x01
#define SOL_IP			0x02

#define SOCK_RAW		0x01
#define SOCK_STREAM		0x02
#define SOCK_DGRAM		0x04
#define SOCK_RDM		0x08
#define SOCK_SEQPACKET	0x10

#define MSG_OOB		0x01
#define MSG_PEEK	0x02
#define MSG_DONTROUTE	0x04
#define MSG_NOSIGNAL	0x08

#define SIOCATMARK		0x01

/* Shutdown defines */
#define SHUT_RD			0x01
#define SHUT_WR			0x02
#define SHUT_RDWR		0x03

#define FIONBIO		100
#define FIONREAD	200

/* Standard socket functions. */
ssize_t recv(int s, void* buf, size_t len, int flags);
ssize_t send(int s, const void* buf, size_t len, int flags);
int socket(int domain, int type, int protocol);
int setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen);
int getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen);
int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);
int accept(int sockfd, struct sockaddr* addr, socklen_t *addrlen);
int connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen);
int shutdown(int s, int how);
int listen(int s, int backlog);

#endif
