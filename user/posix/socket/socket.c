#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <syscalls.h>

#include <stdio.h> //TEMP

ssize_t send(int s, const void* buf, size_t len, int flags)
{
	int ret=SysSocketSend(s, buf, len, flags);

	if (ret < 0)
	{
		errno=-ret;
		return -1;
	}

	return ret;
}

ssize_t recv(int s, void* buf, size_t len, int flags)
{
	int ret=SysSocketReceive(s, buf, len, flags);

	if (ret < 0)
	{
		errno=-ret;
		return -1;
	}

	return ret;
}

int connect(int s, const struct sockaddr* serv_addr, socklen_t addrlen)
{
	int ret=SysSocketConnect(s, serv_addr, addrlen);

	if (ret < 0)
	{
		errno=-ret;
		return -1;
	}

	return ret;
}

int socket(int domain, int type, int protocol)
{
	return SysSocketCreate(domain, type, protocol);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t *addrlen)
{
	int ret=SysSocketAccept(sockfd, addr, addrlen);

	if (ret < 0)
	{
		errno=-ret;
		return -1;
	}

	return ret;
}

int bind(int s, const struct sockaddr* my_addr, socklen_t addr_len)
{
	return SysSocketBind(s, my_addr, addr_len);
}

int shutdown(int s, int how)
{
	return SysSocketClose(s);
}

int listen(int s, int backlog)
{
	return SysSocketListen(s, backlog);
}

int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
	printf("setsockopt\n");
	return 0;
}

/* TODO: Move */
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret = SysPoll(fds, nfds, timeout);

	if (ret < 0)
	{
		errno = -ret;
		ret = -1;
	}

	return ret;
}

unsigned short ntohs(unsigned short s)
{
	printf("ntohs\n");
	return 0;
}

short htons(short s)
{
	printf("htons\n");
	return 0;
}

unsigned long ntohl(unsigned long l)
{
	printf("ntohl\n");
	return 0;
}

long htonl(long l)
{
	printf("htonl\n");
	return 0;
}

int getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen)
{
	printf("getsockopt\n");
	return 0;
}

struct hostent* gethostbyname(const char* name)
{
	printf("gethostbyname(%s)\n", name);
	return NULL;
}

void* getprotobyname(const char* name)
{
	printf("getprotobyname(%s)\n", name);
	return NULL;
}

struct hostent* gethostbyaddr(const void* addr, socklen_t len, int type)
{
	printf("gethostbyaddr()\n");
	return NULL;
}

int inet_pton(int af, const char* src, void* dst)
{
	printf("inet_pton\n");
	return 0;
}
