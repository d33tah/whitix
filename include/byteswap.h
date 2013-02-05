#ifndef BYTESWAP_H
#define BYTESWAP_H

#include <typedefs.h>

static inline DWORD BeToCpu32(DWORD x)
{
	unsigned char* p=(unsigned char*)&x;

	return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline DWORD CpuToBe32(DWORD x)
{
	return BeToCpu32(x);
}

#endif
