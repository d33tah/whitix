#ifndef _SOCKET_H
#define _SOCKET_H

#define REG_PACKET_OPEN_KEY		1
#define REG_PACKET_READ_KEY		2
#define REG_PACKET_ENUM_KEYSET	3
#define REG_PACKET_GET_ENT_TYPE	4

struct RegPacketHeader
{
	int type;
	int length;
};

struct RegOpenPacket
{
	int type;
	int length;
	unsigned long accessRights;
	char name[1024];
};

struct RegReadKeyPacket
{
	int type;
	int length;
	unsigned int handleId;
	char name[1024];
};

struct RegReadKeyReply
{
	int error;
	unsigned int length;
	int type;
};

struct RegEnumSetPacket
{
	int type;
	int length;
	unsigned long handleId;
	unsigned long index;
	unsigned long size;
};

#endif
