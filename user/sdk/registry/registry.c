#include <network.h>
#include <stdio.h>
#include <stdlib.h>
#include <registry.h>

#include <string.h>

char* regDefaultName=":/Registry";

/* Registry packet types. */
#define REG_PACKET_OPEN_KEYSET		1
#define REG_PACKET_READ_KEY		2
#define REG_PACKET_ENUM_KEYSET		3
#define REG_PACKET_GET_ENT_TYPE		4

/* Used for locating an entry. */
struct RegReadKeyPacket
{
	int type;
	int length;
	unsigned long handleId;
	char name[REG_NAME_LENGTH];
};

int RegRegistryOpen(char* name, struct Registry* registry)
{
	int sockFd;
	struct SocketAddr addr;
	
	if (name)
		return -1;
	else	
		name=regDefaultName;
		
	sockFd=NetSocketCreate(PF_LOCAL, 0, 0);
	
	addr.family=PF_LOCAL;
	strcpy(addr.data, name);
	
	NetSocketConnect(sockFd, &addr, sizeof(struct SocketAddr));

	registry->sockFd=sockFd;
	
	return 0;
}

int RegRegistryClose(struct Registry* registry)
{
	NetSocketClose(registry->sockFd);
	return 0;
}

int RegKeySetOpen(struct Registry* registry, char* name, unsigned long accessRights, struct RegKeySet* keySet)
{
	int err;
	
	if (!registry || !keySet)
		return -1;
		
	struct RegOpenPacket
	{
		int type;
		int length;
		unsigned long accessRights;
		char name[REG_NAME_LENGTH];
	};
	
	struct RegOpenPacket packet;
	int handleId;
	
	packet.type=REG_PACKET_OPEN_KEYSET;
	packet.length=sizeof(struct RegOpenPacket);
	packet.accessRights=accessRights;
	strncpy(packet.name, name, REG_NAME_LENGTH);
	
	err=NetSocketSend(registry->sockFd, &packet, sizeof(struct RegOpenPacket), 0);
	
	if (err < 0)
		return err;
		
	NetSocketReceive(registry->sockFd, &handleId, 4, 0);
	
	if (handleId == -1)
		return -1;
	
	keySet->handleId=handleId;
	keySet->registry=registry;
	
	return 0;
}

int RegKeySetClose(struct RegKeySet* keySet)
{
	/* TODO: Send close message. */
	return 0;
}

int RegKeyReadInt(struct RegKeySet* keySet, char* name, int* ret)
{
	int type, length=sizeof(int);
	
	if (RegKeySetReadKey(keySet, name, &type, ret, &length) < 0)
		return -1;
		
	if (type != REG_KEY_TYPE_INT)
		return -1;
		
	return 0;
}

int RegKeyReadString(struct RegKeySet* keySet, char* name, char** data, unsigned long* length)
{
	int type;
	unsigned long rLength=2048;
	
	/* TODO: Check return to see if buf needs to be expanded? */
	
	if (!*data)
		*data=malloc(2048);
		
	if (length)
		rLength=*length;
	
	if (RegKeySetReadKey(keySet, name, &type, *data, &rLength) < 0)
		return -1;
		
	if (type != REG_KEY_TYPE_STR)
		return -1;
	
	if (length)	
		*length=rLength;
	
	return 0;
}

/* TODO: Expand format to multiple keys. */

int RegKeySetReadKey(struct RegKeySet* keySet, char* name, int* type, char* data, unsigned long* length)
{
	int err;
	int sockFd;
	
	if (!keySet || !name)
		return -1;
	
	struct RegReadKeyReply
	{
		int error;
		unsigned int length;
		int type;
	};
	
	struct RegReadKeyPacket packet;
	struct RegReadKeyReply reply;
	
	sockFd=keySet->registry->sockFd;
	
	packet.type=REG_PACKET_READ_KEY;
	packet.length=sizeof(struct RegReadKeyPacket);
	packet.handleId=keySet->handleId;
	strncpy(packet.name, name, REG_NAME_LENGTH);
	
	err=NetSocketSend(sockFd, &packet, sizeof(struct RegReadKeyPacket), 0);
	
	if (err < 0)
		return err;
		
	NetSocketReceive(sockFd, &reply, sizeof(struct RegReadKeyReply), 0);
	
	if (data)
	{
		if (!length)
			return -1;
			
		if (*length < reply.length)
			return -1;
	}
	
	*length=NetSocketReceive(sockFd, data, *length, 0);
	
	if (type)
		*type=reply.type;
	
	return 0;
}

int RegKeySetIterInit(struct RegKeySet* keySet, struct RegKeySetIter* iter)
{
	iter->keySet=keySet;
	iter->bufSize=4096;
	iter->buf=(char*)malloc(4096);
	iter->next=0;
	iter->index=0;
	iter->size=0;
		
	return 0;
}

char* RegKeySetIterNext(struct RegKeySetIter* iter)
{
	struct RegEnumSetPacket
	{
		int type;
		int length;
		unsigned long handleId;
		unsigned long index;
		unsigned long size;
	};
	
	char* ret;
	int bytes;
	int sockFd;
	
	sockFd=iter->keySet->registry->sockFd;
	
	if (!iter)
		return NULL;
		
	if (iter->size <= iter->next)
		iter->size=iter->next=0;
		
	if (iter->size == 0)
	{
		/* Get the next batch of directory entries. */
		struct RegEnumSetPacket packet;
		packet.type=REG_PACKET_ENUM_KEYSET;
		packet.length=sizeof(struct RegEnumSetPacket);
		packet.handleId=iter->keySet->handleId;
		packet.index=iter->index;
		packet.size=iter->bufSize;
		
		bytes=NetSocketSend(sockFd, &packet, packet.length, 0);
		
		if (bytes <= 0)
			return NULL;
		
		bytes=NetSocketReceive(sockFd, iter->buf, iter->bufSize, 0);
		iter->size=bytes;
		
		if (bytes <= 0)
			return NULL;
		
		printf("Got %d bytes\n", bytes);
	}
	
	ret=iter->buf+iter->next;
	iter->next+=strlen(ret)+1;
	iter->index++;
	
	return ret;
}

int RegKeySetIterClose(struct RegKeySetIter* iter)
{
	if (!iter)
		return -1;
		
	free(iter->buf);
	
	return 0;
}

int RegEntryGetType(struct RegKeySet* keySet, char* name, int* ret)
{
	struct RegReadKeyPacket packet;
	int sockFd, err;
	
	sockFd=keySet->registry->sockFd;
	
	packet.type=REG_PACKET_GET_ENT_TYPE;
	packet.length=sizeof(struct RegReadKeyPacket);
	packet.handleId=keySet->handleId;
	strncpy(packet.name, name, REG_NAME_LENGTH);
	
	err=NetSocketSend(sockFd, &packet, sizeof(struct RegReadKeyPacket), 0);
	
	if (err < 0)
		return err;
		
	NetSocketReceive(sockFd, ret, 4, 0);
	
	return 0;
}
