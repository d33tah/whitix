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

/* TODO: check fds, add more memory checks. Cleanup! */

#include <console.h>
#include <error.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <module.h>
#include <net/network.h>
#include <net/socket.h>
#include <slab.h>
#include <task.h>
#include <typedefs.h>
#include <user_acc.h>
#include <vmm.h>
#include <sys.h>

struct Cache* socketCache=NULL;
struct SocketOps* socketOps[NET_FAMILIES_MAX];

int SocketPoll(struct File* file, struct PollItem* item, struct PollQueue* pollQueue);
int SocketFreeVNode(struct VNode* vNode);

struct SuperBlockOps socketSbOps=
{
	.freeVNode = SocketFreeVNode,
};

struct VfsSuperBlock socketSuperBlock=
{
	.sbOps = &socketSbOps
};

/* TODO: Have SocketClose as a file operation? */

struct FileOps sockFileOps=
{
	.poll = SocketPoll,
};

/***********************************************************************
 *
 * FUNCTION: SocketFreeVNode
 *
 * DESCRIPTION: Free the socket vNode, and close the socket.
 *
 * PARAMETERS: vNode - vNode to be freed.
 *
 * RETURNS: 0 on success, -EINVAL on an invalid socket pointer.
 *
 ***********************************************************************/

int SocketFreeVNode(struct VNode* vNode)
{
	struct Socket* socket=SOCKET_GET(vNode);

	/* If SocketClose is called, via SysSocketClose, then the socket pointer
	 * will already be NULL, so the socket won't be closed and freed twice. */
	if (!socket)
		return -EINVAL;

	if (socket->socketOps && socket->socketOps->close)
		socket->socketOps->close(socket);

	MemCacheFree(socketCache, socket);

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: SocketGet
 *
 * DESCRIPTION: Get a socket pointer from a file descriptor.
 *
 * PARAMETERS: fd - file descriptor corresponding to the socket pointer.
 *
 * RETURNS: socket pointer if fd is a valid file descriptor, NULL if not.
 *
 ***********************************************************************/

struct Socket* SocketGet(int fd)
{
	struct File* file;

	file = FileGet(fd);
	
	if (!file)
		return NULL;

	return SOCKET_GET(file->vNode);
}

/***********************************************************************
 *
 * FUNCTION: 	SocketRemoveFd
 *
 * DESCRIPTION: Remove a socket file from the file descriptor array, by
 *				setting the vNode field of the socket's file structure to NULL.
 *
 * PARAMETERS: 	socket - the socket that links to the file to be removed.
 *
 * RETURNS: 	Nothing.
 *
 ***********************************************************************/

void SocketRemoveFd(struct Socket* socket)
{
	struct File* file;

	file = FileGet(socket->fd);

	if (UNLIKELY(!file))
		return;
}

struct Socket* SocketCreate(int domain, int type)
{
	struct Socket* socket;

	if (!socketOps[domain])
		return NULL;

	socket=MemCacheAlloc(socketCache);
	if (!socket)
		return NULL;

	socket->family=domain;
	socket->type=type;
	socket->parent=NULL;
	socket->socketOps=socketOps[domain];
	INIT_LIST_HEAD(&socket->recvPackets);
	INIT_LIST_HEAD(&socket->sendPackets);
	INIT_LIST_HEAD(&socket->childSocks);

	/* Protocol may want to allocate data depending on type. */
	if (socket->socketOps->create)
	{
		if (socket->socketOps->create(socket))
		{
			/* Create failed. (Ran out of memory or illegal socket type). */
			MemCacheFree(socketCache, socket);
			return NULL;
		}
	}

	return socket;
}

int SocketClose(struct Socket* socket)
{
	struct Socket* curr, *currSafe;

	if (!socket)
		return -EFAULT;

	if (socket->socketOps->close)
		socket->socketOps->close(socket);

	if (socket->parent)
		ListRemove(&socket->next);
	else if (socket->type & SOCKET_TYPE_SERVER)
	{
		ListForEachEntrySafe(curr, currSafe, &socket->childSocks, next)
		{
			SocketRemoveFd(curr);
			SocketClose(curr);
		}
	}

	MemCacheFree(socketCache, socket);
		
	return 0;
}

int SocketSetupChild(struct Socket* parent, struct Socket* child)
{
	/* Duplicate much of the server socket's data. */
	child->parent=parent;
	child->state=parent->state;
	child->family=parent->family;
	child->type=parent->type & ~SOCKET_TYPE_SERVER;
	memcpy(&child->addr, &parent->addr, /* TODO: length */ sizeof(struct SocketAddr));
	child->socketOps=parent->socketOps;
	INIT_LIST_HEAD(&child->recvPackets);
	INIT_LIST_HEAD(&child->sendPackets);
	child->state=SOCKET_STATE_CONNECTED;

	/* List is so when the server is closed, its children can be closed too. */
	ListAddTail(&child->next, &parent->childSocks);

	return 0;
}

/* Make local-only? */
int SocketDoAccept(struct Socket* server, struct Socket* client, struct Socket** child)
{
	struct Socket* newSocket;
	int newFd;

	client->state = SOCKET_STATE_CONNECTED;
	
	/* Spawn new socket to deal with this client. TODO: Remove Sys.. */
	newFd=SysSocketCreate(client->family, client->type, 0);
	if (newFd < 0)
		return newFd;

	newSocket=SOCKET_GET(current->files[newFd]->vNode);
	SocketSetupChild(server, newSocket);
	*child=newSocket;

	return newFd;
}

SYMBOL_EXPORT(SocketDoAccept);

struct SocketBuffer* SocketAllocateBuffer(const void* buffer, int length)
{
	struct SocketBuffer* sockBuff;

	sockBuff=(struct SocketBuffer*)MemAlloc(sizeof(struct SocketBuffer) + length);
	if (!sockBuff)
		return NULL;

	sockBuff->length=length;
	sockBuff->read=0;
	memcpy(sockBuff->data, (void*)buffer, length);
	return sockBuff;
}

SYMBOL_EXPORT(SocketAllocateBuffer);

int SocketDestroyBuffer(struct SocketBuffer* sockBuff)
{
	ListRemove(&sockBuff->next);
	MemFree(sockBuff);
	return 0;
}

SYMBOL_EXPORT(SocketDestroyBuffer);

int SocketPoll(struct File* file, struct PollItem* item, struct PollQueue* pollQueue)
{
	struct Socket* socket=SOCKET_GET(file->vNode);

	if (!socket->socketOps || !socket->socketOps->poll)
		return -ENOTIMPL;

	return socket->socketOps->poll(socket, item, pollQueue);
}

/* FIXME: Why is this needed? */
int SocketCheckArea(void* buffer, DWORD length)
{
	struct VMArea* base, *curr;
	DWORD verifiedLen=0;

	base=VmLookupAddress(current, (DWORD)buffer);
	if (!base)
		return -EFAULT;

	verifiedLen=(base->start+base->length)-(DWORD)buffer;
	if (verifiedLen > length)
		return length;

	while (verifiedLen < length)
	{
		curr=ListEntry(base->list.next,struct VMArea,list);
		/* Reached the end of the VMArea list. */
		if (&curr->list == &current->areaList)
			break;

		/* There must be no breaks between areas. */
		if (curr->start != base->start+base->length)
			break;

		verifiedLen+=MIN(length-verifiedLen, curr->length);
		base=curr;
	}

	return verifiedLen;
}

int SocketCreateFile(struct File* file, struct Socket* socket)
{
	file->vNode = VNodeGetEmpty();
	file->fileOps = file->vNode->fileOps=&sockFileOps;
	file->vNode->extraInfo = (void*)socket;
	file->vNode->superBlock = &socketSuperBlock;

	return 0;
}

/* System calls. Move to separate file. */

int SysSocketCreate(int domain, int type, int flags)
{
	struct Socket* socket;
	int sockFd;

	if ((domain < 0) || (domain > NET_FAMILIES_MAX))
		return -EFAMILY;

	socket=SocketCreate(domain, type);

	if (!socket)
		return -ENOMEM;

	sockFd = VfsGetFreeFd(current);

	if (sockFd < 0)
		return sockFd;
		
	current->files[sockFd] = FileAllocate();

	SocketCreateFile(current->files[sockFd], socket);

	return sockFd;
}

int SysSocketBind(int fd, struct SocketAddr* addr, int length)
{
	struct Socket* socket;
	int ret;

	socket=SocketGet(fd);

	if (!socket)
		return -EBADF;

	if (!socket->socketOps->bind)
		return -ENOTIMPL;

	ret=socket->socketOps->bind(socket, addr, length);

	if (ret)
		return ret;

	socket->state=SOCKET_STATE_BOUND;
	socket->type|=SOCKET_TYPE_SERVER;

	return 0;
}

int SysSocketConnect(int fd, struct SocketAddr* addr, int length)
{
	struct Socket* socket;

	socket = SocketGet(fd);

	if (!socket)
		return -EBADF;

	if (!socket->socketOps || !socket->socketOps->connect)
		return -ENOTIMPL;

	return socket->socketOps->connect(socket, addr, length);
}

int SysSocketListen(int fd, int backlog)
{
	struct Socket* socket;

	socket=SocketGet(fd);

	if (!socket)
		return -EBADF;

	if (socket->state != SOCKET_STATE_BOUND)
		return -EOPNOTSUPP;

	if (!socket->socketOps->listen)
		return -ENOTIMPL;

	return socket->socketOps->listen(socket, backlog);
}

int SysSocketAccept(int fd, struct SocketAddr* addr, int* length)
{
	struct Socket* socket;

	socket=SocketGet(fd);

	if (!socket)
		return -EBADF;
		
	if (socket->state != SOCKET_STATE_ACCEPTING)
		return -EOPNOTSUPP;

	if (!socket->socketOps->accept)
		return -ENOTIMPL;

	return socket->socketOps->accept(socket, addr, length);
}

int SysSocketSend(int fd, const void* buffer, int length, int flags)
{
	struct Socket* socket;

	socket=SocketGet(fd);

//	KePrint("%s: SysSocketSend(%d, %d), %#X, %#X\n", current->name, fd, length, socket->socketOps->send, socket);

	if (!socket)
		return -EBADF;

	if (socket->state != SOCKET_STATE_CONNECTED)
		return -EOPNOTSUPP;

	if (!socket->socketOps || !socket->socketOps->send)
		return -ENOTIMPL;

	if (SocketCheckArea((void*)buffer, length) < 0)
		return -EFAULT;

	return socket->socketOps->send(socket, buffer, length, flags);
}

int SysSocketReceive(int fd, void* buffer, int length, int flags)
{
	struct Socket* socket;

	socket=SocketGet(fd);

//	KePrint("%s: SysSocketReceive(%d, %d)\n", current->name, fd, length);

	if (!socket)
		return -EBADF;

	if (socket->state != SOCKET_STATE_CONNECTED)
		return -EOPNOTSUPP;

	if (!socket->socketOps->receive)
		return -ENOTIMPL;

	if (SocketCheckArea(buffer, length) < 0)
		return -EFAULT;

	return socket->socketOps->receive(socket, buffer, length, flags);
}

int SysSocketIoCtl(int fd, unsigned long code, void* data)
{
	return -ENOTIMPL;
}

int SysSocketClose(int fd)
{
	struct Socket* socket;
	struct File* file=FileGet(fd);
	struct VNode* vNode;

	if (!file)
		return -EBADF;

	vNode=file->vNode;
	socket=SOCKET_GET(vNode);
	vNode->extraInfo=NULL; /* So VNodeRelease doesn't try to release something it can't. */

	return SocketClose(socket);
}

int SocketRegisterFamily(int family, struct SocketOps* sockOps)
{
	if (socketOps[family])
		return -EEXIST;

	socketOps[family]=sockOps;
	return 0;
}

SYMBOL_EXPORT(SocketRegisterFamily);

struct SysCall netSysCalls[]=
{
	SysEntry(SysSocketCreate, 12),
	SysEntry(SysSocketBind, 12),
	SysEntry(SysSocketConnect, 12),
	SysEntry(SysSocketListen, 8),
	SysEntry(SysSocketAccept, 12),
	SysEntry(SysSocketSend, 16),
	SysEntry(SysSocketReceive, 16),
	SysEntry(SysSocketIoCtl, 12),
	SysEntry(SysSocketClose, 4),
	SysEntryEnd()
};

int SocketInit()
{
	/* Create socket cache */
	socketCache=MemCacheCreate("Sockets", sizeof(struct Socket), NULL, NULL, 0);
	if (!socketCache)
		return -ENOMEM;

	/* Register system calls. */
	SysRegisterRange(NET_SYS_BASE, netSysCalls);

	return 0;
}

