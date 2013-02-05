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

/* Just detects whether the system is or is not a SMP system */

#include <console.h>
#include <i386/smp.h>
#include <typedefs.h>
#include <print.h>

#define MP_FPS_SIG   (('_' << 24) | ('P' << 16) | ('M' << 8) | ('_'))

struct MpConfig;

struct MpFloating
{
	char sig[4];
	struct MpConfig* config;
	BYTE length; /* In 16 byte paragraphs */
	BYTE version; /* 1 = 1.1, 4 = 1.4 */
	BYTE checkSum; /* Should be zero */
	BYTE mpFeatures1;
	BYTE mpFeatures2;
	BYTE reserved[3];
};

struct MpConfig
{
	char sig[4];
	WORD length;
	BYTE version,checkSum;
	char oemId[8];
	char productId[12];
	DWORD tablePointer,tableSize;
	BYTE entryCount;
	DWORD apicAddress;
	WORD extTableLength;
	BYTE extTableCheckSum;
};

/* SmpDetectStruct - Searches an area of memory for the "_MP_" signature */
DWORD SmpDetectStruct(DWORD addr,DWORD length)
{
	DWORD* i;
	for (i=(DWORD*)addr; i<(DWORD*)(addr+length); i+=4)
		if (*i == MP_FPS_SIG)
			return (DWORD)i;

	return 0;
}

int SmpInit()
{
	DWORD addr;
	extern DWORD ebdaAddr;

	/* 
	 * The MP Floating Pointer Structure is in one of four memory areas: 
	 * (1) The first kilobyte of the Extended BIOS Data Area (EBDA). 
	 * (2) The last kilobyte of base memory (639-640k). 
	 * (3) The BIOS ROM address space (0xF0000-0xFFFFF). 
	 * (4) The first kilobyte of memory.
	 * The OS should search for the ASCII string "_MP_" in these four areas
	 */

//	if ((addr=SmpDetectStruct(0x0, 0x400)))
//		goto found;

	if ((addr=SmpDetectStruct(0x9FC00,0x400)))
		goto found;

	if ((addr=SmpDetectStruct(0xF0000,0xFFFF)))
		goto found;

	if (ebdaAddr && (addr=SmpDetectStruct(ebdaAddr,0x400)))
		goto found;

	return 0;

found:
	KePrint(KERN_INFO "SMP: MP table found at %#X\n",addr);
	return 0;
}
