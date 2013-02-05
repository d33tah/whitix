/* Network and socket library. */
#include <socket.h>
#include <syscalls.h>

int NetSocketCreate(int domain, int type, int protocol)
{
	return SysSocketCreate(domain, type, protocol);
}

int NetSocketBind(int fd, struct SocketAddr* address, int length)
{
	return SysSocketBind(fd, address, length);
}

int NetSocketListen(int fd, int backlog)
{
	return SysSocketListen(fd, backlog);
}

int NetSocketAccept(int fd, struct SocketAddr* address, int* length)
{
	return SysSocketAccept(fd, address, length);
}

int NetSocketConnect(int fd, struct SocketAddr* address, int length)
{
	return SysSocketConnect(fd, address, length);
}

int NetSocketSend(int fd, const void* buffer, int size, int flags)
{
	return SysSocketSend(fd, buffer, size, flags);
}

int NetSocketReceive(int fd, void* buffer, int size, int flags)
{
	return SysSocketReceive(fd, buffer, size, flags);
}

int NetSocketClose(int fd)
{
	return SysSocketClose(fd);
}
