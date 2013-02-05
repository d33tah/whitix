/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <llist.h>
#include <fs/vfs.h>

/* Forward declarations. */
struct NetDevice;

/* Defines */
#define SOCKET_STATE_FREE		0
#define SOCKET_STATE_BOUND		1
#define SOCKET_STATE_ACCEPTING	2
#define SOCKET_STATE_WAITING	3 /* For server to accept. */
#define SOCKET_STATE_CONNECTED	4

#define SOCKET_TYPE_STREAM		0
#define SOCKET_TYPE_SERVER		(1 << 31)

#define SOCKET_DATA_LENGTH		20

#define SOCKET_GET(vNode) ((struct Socket*)((vNode)->extraInfo))

/* Structures */
struct SocketAddr
{
	unsigned int family;
	char data[SOCKET_DATA_LENGTH];
};

struct Socket
{
	int state;
	int family;
	int type;
	struct SocketAddr addr;
	struct SocketOps* socketOps;

	/* Since a server socket and server's children sockets are used for
	 * entirely different purposes, we can optimize structure size by
	 * using a union.
	 */
//		struct {
			struct ListHead sendPackets, recvPackets;
//		};

		struct ListHead childSocks;
		
	struct ListHead next;
	struct Socket* parent;
	int fd;
	struct NetDevice* device;
	void* protoInfo;
};

struct SocketBuffer
{
	int length;
	int read;
	struct ListHead next;
	char data[1];
};

struct SocketOps
{
	int (*bind)(struct Socket* socket, struct SocketAddr* addr, int length);
	int (*connect)(struct Socket* socket, struct SocketAddr* addr, int length);
	int (*create)(struct Socket* socket);
	int (*close)(struct Socket* socket);
	int (*listen)(struct Socket* socket, int backlog);
	int (*accept)(struct Socket* socket, struct SocketAddr* addr, int* length);
	int (*send)(struct Socket* socket, const void* buffer, int length, int flags);
	int (*receive)(struct Socket* socket, void* buffer, int length, int flags);
	int (*poll)(struct Socket* socket, struct PollItem* item, struct PollQueue* pollQueue);
};

/* Prototypes */
int SocketRegisterFamily(int family, struct SocketOps* ops);
int SocketInit();
int SocketClose(struct Socket* socket);
int SocketSetupChild(struct Socket* parent, struct Socket* child);
int SocketDoAccept(struct Socket* server, struct Socket* client, struct Socket** child);
struct SocketBuffer* SocketAllocateBuffer(const void* buffer, int length);
int SocketDestroyBuffer(struct SocketBuffer* sockBuff);
struct Socket* SocketCreate(int domain, int type);
int SocketCreateFile(struct File* file, struct Socket* socket);

/* System calls */
int SysSocketCreate(int domain, int type, int protocol);
int SysSocketBind(int fd, struct SocketAddr* addr, int length);
int SysSocketConnect(int fd, struct SocketAddr* addr, int length);
int SysSocketListen(int fd, int backlog);
int SysSocketAccept(int fd, struct SocketAddr* addr, int* length);
int SysSocketSend(int fd, const void* buffer, int length, int flags);
int SysSocketReceive(int fd, void* buffer, int length, int flags);
int SysSocketIoCtl(int fd, unsigned long code, void* data);
int SysSocketClose(int fd);

#endif
