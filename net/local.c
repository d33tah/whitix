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

#include <console.h>
#include <error.h>
#include <net/network.h>
#include <net/socket.h>
#include <slab.h>
#include <sys.h>
#include <task.h>
#include <typedefs.h>
#include <wait.h>
#include <print.h>
#include <module.h>

struct LocalSocketAddr
{
	unsigned int family;
	char name[SOCKET_DATA_LENGTH];
};

struct LocalSockInfo
{
//	union {
		struct Socket* peer; /* Used by child sockets and clients. */
		struct ListHead waitingSocks; /* Used by the parent server socket. */
//	};

	WaitQueue waitQueue;
};

LIST_HEAD(localListenSockets);
struct Cache* localInfoCache=NULL;

/***********************************************************************
 *
 * FUNCTION: 	LocalSetPeer
 *
 * DESCRIPTION: Set one socket as the peer (or endpoint) of another. This
 *				function must be called by the local socket code before 
 *				sending data on the socket.
 *
 * PARAMETERS: 	first -	the socket whose peer value will be set.
 *				second - the peer in question. It can be NULL if the first
 *				socket is in the process of closing.
 *
 * RETURNS: 	Nothing.
 *
 ***********************************************************************/

static void LocalSetPeer(struct Socket* first, struct Socket* second)
{
	struct LocalSockInfo* info;

	if (!first)
		return;

	info=(struct LocalSockInfo*)(first->protoInfo);
	
	if (!info)
		return;

	info->peer=second;
}

/***********************************************************************
 *
 * FUNCTION: 	LocalBind
 *
 * DESCRIPTION: Prepare the socket as a server parent socket. LocalBind
 *				copies the local network address and sets up the list of
 *				sockets that are waiting for a connection to it.
 *
 * PARAMETERS: 	socket - the socket being bound to an address
 *
 * RETURNS: 	-EFAULT - if socket->protoInfo is an invalid value. 0
 *				otherwise.
 *
 ***********************************************************************/

int LocalBind(struct Socket* socket, struct SocketAddr* addr, int length)
{
	struct LocalSockInfo* info=(struct LocalSockInfo*)(socket->protoInfo);
	
	memcpy(&socket->addr, addr, length);
	INIT_LIST_HEAD(&info->waitingSocks);

	return 0;
}

int LocalConnect(struct Socket* socket, struct SocketAddr* addr, int length)
{
	struct LocalSocketAddr* socketAddr=(struct LocalSocketAddr*)addr;
	struct LocalSocketAddr* serverSockAddr;
	struct LocalSockInfo* serverInfo, *clientInfo;
	struct Socket* curr, *curr2, *serverSock=NULL;

	/* Can only connect to local sockets. */
	if (socketAddr->family != PF_LOCAL)
		return -EINVAL;

	/* Find server socket. */
	ListForEachEntrySafe(curr, curr2, &localListenSockets, next)
	{
		serverSockAddr=(struct LocalSocketAddr*)&curr->addr;
		if (!strncmp(socketAddr->name, serverSockAddr->name, SOCKET_DATA_LENGTH))
		{
			serverSock=curr;
			break;
		}
	}

	if (!serverSock)
		return -ENOENT;

	clientInfo = (struct LocalSockInfo*)(socket->protoInfo);

	serverInfo=(struct LocalSockInfo*)(serverSock->protoInfo);
	socket->state=SOCKET_STATE_WAITING;

	/* And if it is a blocking socket, wait.
	 * Add self to the server's waiting sockets, so Accept can pick things up.
	 */
	 
	ListAdd(&socket->next, &serverInfo->waitingSocks);	 

	WakeUp(&serverInfo->waitQueue);

	WAIT_ON(&clientInfo->waitQueue, socket->state == SOCKET_STATE_CONNECTED);
		
	return 0;
}

int LocalCreate(struct Socket* socket)
{
	/* Allocate the protocol-specific information.
	 * The structure is filled in later by the bind or connect functions.
	 */
	struct LocalSockInfo* info;
	
	if (socket->type & SOCKET_TYPE_STREAM)
		return -ESOCKTYPE;
	
	info=(struct LocalSockInfo*)MemCacheAlloc(localInfoCache);

	if (!info)
		return -ENOMEM;

	info->peer=NULL;
	INIT_WAITQUEUE_HEAD(&info->waitQueue);

	socket->protoInfo=info;

	return 0;
}

int LocalClose(struct Socket* socket)
{
	struct LocalSockInfo* info;

	if (!socket)
		return 0;

	info=(struct LocalSockInfo*)(socket->protoInfo);

	if (!info)
		return 0;
	
	if (!(socket->type & SOCKET_TYPE_SERVER))
		/* Inform peer of closure. */
		LocalSetPeer(info->peer, NULL);

	return 0;
}

int LocalListen(struct Socket* socket, int backlog)
{
	/* TODO: Take note of backlog. */
	ListAdd(&socket->next, &localListenSockets);
	socket->state=SOCKET_STATE_ACCEPTING;
	return 0;
}

int LocalAccept(struct Socket* socket, struct SocketAddr* addr, int* length)
{
	struct Socket* client, *child;
	struct LocalSockInfo* info=(struct LocalSockInfo*)(socket->protoInfo);
	struct LocalSockInfo* clientInfo;
	int ret;

	/* Wait for a connection. TODO: Should time out. */
	WAIT_ON(&info->waitQueue, !ListEmpty(&info->waitingSocks));

	client=ListEntry(info->waitingSocks.next, struct Socket, next);
	
	ListRemove(&client->next);

	ret=SocketDoAccept(socket, client, &child);
	
	if (ret < 0)
		return ret;
	
	if (addr)
	{
		/* Local client sockets don't have a name. */
		addr->family=PF_LOCAL;
		ZeroMemory(addr->data, SOCKET_DATA_LENGTH);
	}
	
	INIT_WAITQUEUE_HEAD(&info->waitQueue);
	
	if (length)
		*length=sizeof(unsigned int);
	
	/* Make the child server socket and client socket each other's peers. */
	LocalSetPeer(child, client);
	LocalSetPeer(client, child);

	clientInfo = (struct LocalSockInfo*)(client->protoInfo);
	WakeUp(&clientInfo->waitQueue);

	return ret;
}

int LocalSend(struct Socket* socket, const void* buffer, int length, int flags)
{
	struct LocalSockInfo* info=(struct LocalSockInfo*)(socket->protoInfo);
	struct Socket* peer;
	struct LocalSockInfo* peerInfo;
	struct SocketBuffer* sockBuff;
	DWORD iFlags;

	if (socket->state != SOCKET_STATE_CONNECTED)
		return -EINVAL;
	
	peer=info->peer;

	/* The other end has been closed. */
	if (!peer)
		return -ECONNRESET;
	
	/* Allocate socket buffer. */
	sockBuff=SocketAllocateBuffer(buffer, length);
	if (!sockBuff)
		return -ENOMEM;

	peerInfo=(struct LocalSockInfo*)(peer->protoInfo);
	
	IrqSaveFlags(iFlags);
	/* Add to peer's receive queue. */
	ListAddTail(&sockBuff->next, &peer->recvPackets);
	IrqRestoreFlags(iFlags);
	
	WakeUp(&peerInfo->waitQueue);

	return length;
}

int LocalReceive(struct Socket* socket, void* buffer, int length, int flags)
{
	struct SocketBuffer* sockBuff;
	struct LocalSockInfo* info, *peerInfo;
	char* pos, *orig;
	int toRead=0;
	
	pos=orig=(char*)buffer;

	info=(struct LocalSockInfo*)(socket->protoInfo);
	peerInfo=(struct LocalSockInfo*)(info->peer->protoInfo);

	if (ListEmpty(&socket->recvPackets))
		WAIT_ON(&info->waitQueue, !ListEmpty(&socket->recvPackets));

	/* Go through packets until we've got enough. */
	while ((pos-orig) < length)
	{
		if (ListEmpty(&socket->recvPackets))
			break;
		
		PreemptDisable();
		sockBuff=ListEntry(socket->recvPackets.next,struct SocketBuffer, next);

		toRead=MIN(sockBuff->length-sockBuff->read, length);
		memcpy(pos, sockBuff->data, toRead);

		sockBuff->read+=toRead;	
		pos+=toRead;

		if (sockBuff->read == sockBuff->length)
			SocketDestroyBuffer(sockBuff);
			
		PreemptEnable();
	}
	
	return pos-orig;
}

int LocalPoll(struct Socket* socket, struct PollItem* item, struct PollQueue* pollQueue)
{
	struct LocalSockInfo* info=(struct LocalSockInfo*)(socket->protoInfo);
	
	PollAddWait(pollQueue, &info->waitQueue);
	
	if ((socket->type & SOCKET_TYPE_SERVER) && !ListEmpty(&info->waitingSocks))
		item->revents |= POLL_IN;
		
	if ((!(socket->type & SOCKET_TYPE_SERVER) && !ListEmpty(&socket->recvPackets)))
		item->revents |= POLL_IN;

	item->revents |= POLL_OUT;

	if (!(socket->type & SOCKET_TYPE_SERVER) && !info->peer)
		item->revents |= POLL_ERR;

	return 0;
}

struct SocketOps localOps=
{
	.bind = LocalBind,
	.connect = LocalConnect,
	.create = LocalCreate,
	.close = LocalClose,
	.listen = LocalListen,
	.accept = LocalAccept,
	.send = LocalSend,
	.receive = LocalReceive,
	.poll = LocalPoll,
};

int LocalInit()
{
	localInfoCache=MemCacheCreate("LocalSockets", sizeof(struct LocalSockInfo), NULL, NULL, 0);
	if (!localInfoCache)
		return -ENOMEM;

	KePrint("LOCAL: Local network sockets set up.\n");

	SocketRegisterFamily(PF_LOCAL, &localOps);	
	return 0;
}

ModuleInit(LocalInit);
