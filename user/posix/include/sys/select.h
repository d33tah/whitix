#ifndef _POSIX_SELECT_H
#define _POSIX_SELECT_H

#include <sys/types.h>
#include <sys/time.h>

#define _FD_MAXIMUM	256
#define FD_SETSIZE _FD_MAXIMUM/8

typedef struct
{
	int fd_count;
	unsigned char fd_array[FD_SETSIZE];
}fd_set;

#define FD_SET(fd, set) ((set)->fd_array[(fd)/8] |= (1 << ((fd) & 7)))
#define FD_CLR(fd, set) ((set)->fd_array[(fd)/8] &= ~(1 << ((fd) & 7)))
#define FD_ISSET(fd, set) ((set)->fd_array[(fd)/8] & (1 << ((fd) & 7)))
#define FD_ZERO(set) (memset(set, 0, FD_SETSIZE))

int select(int nfds,fd_set* readfds,fd_set* writefds,fd_set* exceptfds,struct timeval* timeout);

#endif
