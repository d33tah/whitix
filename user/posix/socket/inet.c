#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h> //TEMP

in_addr_t inet_addr(const char* cp)
{
	printf("inet_addr\n");
	return 0;
}

int gethostname(char* name, size_t len)
{
	strncpy(name, "WhitixMachine", len);
	return 0;
}

int getpeername(int s, struct sockaddr* name, socklen_t* namelen)
{
	printf("getpeername\n");
	return 0;
}

int getsockname(int s, struct sockaddr* name, socklen_t* namelen)
{
	printf("getsockname\n");
	return 0;
}

ssize_t recvfrom(int s, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen)
{
	printf("recvfrom\n");
	return 0;
}

ssize_t sendto(int s, const void* buf, size_t len, int flags, const struct sockaddr* to, socklen_t tolen)
{
	printf("sendto\n");
	return 0;
}
