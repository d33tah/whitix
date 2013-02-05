#include <network.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* TODO: Use Whitix threading API. */
#include <syscalls.h>

#include "socket.h"
#include "key.h"
#include "keyset.h"
#include "handle.h"

int serverSock;

void RegSocketCreate()
{
	struct SocketAddr addr;
	
	serverSock=NetSocketCreate(PF_LOCAL, 0, 0);
	addr.family=PF_LOCAL;
	strcpy(addr.data, ":/Registry");
	NetSocketBind(serverSock, &addr, sizeof(struct SocketAddr));
	NetSocketListen(serverSock, 10);
}

void RegSocketParseOpen(int clientFd, char* buf, struct RegKeySetHandle** list)
{
	struct RegOpenPacket* packet=(struct RegOpenPacket*)buf;
	struct RegKeySetHandle* handle;
	int handleId;
	
	handle=RegKeySetHandleCreate(packet->name, list);
	
	if (!handle)
	{
		handleId=-1;
		goto send;
	}
	
	handleId=RegKeySetGetHandleId(*list, handle);

send:
	/* Send the handle ID back to the calling application. */
	NetSocketSend(clientFd, &handleId, 4, 0);
}

void RegSocketParseRead(int clientFd, char* buf, struct RegKeySetHandle* list)
{
	struct RegReadKeyPacket* packet=(struct RegReadKeyPacket*)buf;
	struct RegKeySetHandle* handle;
	struct RegKey* key=NULL;
	struct RegReadKeyReply reply;
	char* data;
		
	handle=RegKeySetGetHandle(list, packet->handleId);
	
	if (!handle)
		goto error;
	
	key=RegKeySetFindKey(handle->set, packet->name);
	
	/* Construct a reply, and then send the data. */
	
	if (key)
	{
		reply.error=0;
		
		switch (key->type)
		{
			case REG_KEY_TYPE_STR:
				data=key->sValue;
				reply.length=strlen(key->sValue);
				break;
				
			case REG_KEY_TYPE_INT:
				data=&key->iValue;
				reply.length=sizeof(int);
				break;
		}
		
		reply.type=key->type;
	}else{
error:
		reply.error=-1;
		reply.length=0;
		reply.type=-1;
	}
	
	NetSocketSend(clientFd, &reply, sizeof(struct RegReadKeyReply), 0);
	
	if (key)
		NetSocketSend(clientFd, data, reply.length, 0);
}

void RegSocketParseEnum(int clientFd, char* buf, struct RegKeySetHandle* list)
{
	struct RegEnumSetPacket* packet=(struct RegEnumSetPacket*)buf;
	struct RegKeySetHandle* handle;
	struct RegKeySet* set;
	char* sendBuf, *curr;
	int length=0;
	
	handle=RegKeySetGetHandle(list, packet->handleId);
	
	if (!handle)
		goto error;
		
	set=handle->set;
	
	if (!set)
		goto error;
		
	sendBuf=(char*)malloc(packet->size);
	curr=sendBuf;
	
	if (!sendBuf)
		goto error;
	
//	printf("%s: %d, %d %d\n", set->name, packet->index, set->numChildren, set->numKeys);
	
	/* Iterate through child keysets first, and then child keys. */
	if (packet->index < set->numChildren)
	{
		struct RegKeySet* currChild=RegKeySetGetChild(set->children, packet->index);
		
		while (packet->size > 0 && currChild)
		{
			int strLen=strlen(currChild->name);
			packet->index++;
			strcpy(curr, currChild->name);
			curr+=strLen+1;
			packet->size-=(strLen+1);
			
			currChild=currChild->next;
		}
	}
	
	if (packet->index >= set->numChildren
		&& packet->index < set->numChildren+set->numKeys)
	{
		packet->index-=set->numChildren;
		
		struct RegKey* currKey=RegKeySetGetKey(set, packet->index);

		while (packet->size > 0 && currKey)
		{
			int strLen=strlen(currKey->key);
			packet->index++;
			strcpy(curr, currKey->key);
			curr+=strLen+1;
			packet->size-=(strLen+1);

			currKey=currKey->next;
		}
	}
	
	length=curr-sendBuf;
	
//	printf("Sending %d\n", length);
	
	NetSocketSend(clientFd, sendBuf, length, 0);
	
	free(sendBuf);
	return;
	
error:
	length=-1;
	NetSocketSend(clientFd, &length, sizeof(int), 0);
}

void RegSocketParseType(int clientFd, char* buf, struct RegKeySetHandle* list)
{
	struct RegReadKeyPacket* packet=(struct RegReadKeyPacket*)buf;
	struct RegKeySetHandle* handle;
	struct RegKeySet* set;
	struct RegKey* key;
	int type=-1;

	handle=RegKeySetGetHandle(list, packet->handleId);
	
	if (!handle)
		goto out;
		
	set=handle->set;
	
	key=RegKeySetFindKey(set, packet->name);
	
	if (key)
	{
		type=0;
		goto out;
	}
	
	set=RegKeySetLookupOne(set, packet->name, strlen(packet->name));
	
	if (set)
		type=1;
	
out:
	NetSocketSend(clientFd, &type, sizeof(int), 0);
}

void RegSocketListen(int* clientSock)
{
	int clientFd=*clientSock;
	char buf[4096];
	struct RegPacketHeader* header;
	struct RegKeySetHandle* list=NULL;
	
	while (1)
	{
		int bytesRead=NetSocketReceive(clientFd, buf, 4096, 0);
		
//		printf("bytesRead = %d\n", bytesRead);
		
		if (bytesRead < 0)
			continue;
			
		header=(struct RegPacketHeader*)buf;
		
		/* Check length. Need to read more? */
		
		switch (header->type)
		{
			case REG_PACKET_OPEN_KEY:
				RegSocketParseOpen(clientFd, buf, &list);
				break;
				
			case REG_PACKET_READ_KEY:
				RegSocketParseRead(clientFd, buf, list);
				break;
			
			case REG_PACKET_ENUM_KEYSET:
				RegSocketParseEnum(clientFd, buf, list);
				break;
				
			case REG_PACKET_GET_ENT_TYPE:
				RegSocketParseType(clientFd, buf, list);
				break;
			
			default:
				printf("Regserver: %u\n", header->type);
				continue;
		}
	}
}

void RegSocketAcceptConn()
{
	int clientSock=NetSocketAccept(serverSock, NULL, NULL);
	
	if (clientSock > 0)
		SysCreateThread(RegSocketListen, 0, &clientSock);
}
