#ifndef _POSIX_POLL_H
#define _POSIX_POLL_H

struct pollfd
{
	int fd;
	short int events,revents;
};

typedef unsigned int nfds_t;

int poll(struct pollfd* fds,nfds_t count,int timeout);

/* Defines */
#define POLLIN		0x01
#define POLLOUT		0x02
#define POLLERR		0x04
#define POLLHUP		0x08
#define POLLNVAL	4

#endif
