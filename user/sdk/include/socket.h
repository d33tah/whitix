#ifndef SOCKET_H
#define SOCKET_H

/* Defines */
#define SOCK_STREAM		0

/* Structures */

struct SocketAddr
{
	unsigned long int family;
	unsigned char data[20];
};

/* Function protoypes */
int NetSocketCreate(int domain, int type, int protocol);
int NetSocketBind(int fd, struct SocketAddr* address, int length);
int NetSocketListen(int fd, int backlog);
int NetSocketAccept(int fd, struct SocketAddr* address, int* length);
int NetSocketConnect(int fd, struct SocketAddr* address, int length);
int NetSocketSend(int fd, const void* buffer, int size, int length);
int NetSocketReceive(int fd, void* buffer, int size, int length);
int NetSocketClose(int fd);

#endif
