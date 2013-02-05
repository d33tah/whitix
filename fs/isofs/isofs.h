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

#ifndef ISOFS_H
#define ISOFS_H

#include <typedefs.h>

/* ISO9660 stores little and big-endian versions of the same data. */

struct Both32
{
	DWORD native,foreign;
}PACKED;

typedef struct Both32 Both32;

struct Both16
{
	WORD native,foreign;
}PACKED;

typedef struct Both16 Both16;

struct IsoVolumeDesc
{
	BYTE type,id[5],version,data[2041];
}PACKED;

struct IsoDirEntry
{
	BYTE entryLength,extAddrLength;
	Both32 firstSector;
	Both32 length;
	char date[7];
	BYTE flags,fileUnitSize,interleave;
	Both16 volSeqNum;
	BYTE nameLen;
	char name[0];
}PACKED;

/* Directory entries are padded to an even size, so we can save a bit on the offset. */
#define ISO_TO_VNUM(sector, offset) (((sector) << 11) | (((offset) & 0xFFF) >> 1))
#define ISO_VNUM_SECTOR(vNo) ((vNo) >> 11)
#define ISO_VNUM_OFFSET(vNo) (((vNo) & 0x7FF) << 1)

struct IsoDateTime
{
	BYTE year[4],month[2],day[2],hour[2],minute[2],second[2],sec100[2],gmtOffset;
}PACKED;

struct IsoPrimaryDesc
{
	BYTE type,id[5], version, unused[1];
	char systemId[32], volId[32];
	char unused2[8];
	Both32 numSectors;
	char unused3[32];
	Both16 volSetSize,volSeqNum,sectorSize;
	Both32 pathTableLen;
	DWORD lePathTable1,lePathTable2,bePathTable1,bePathTable2;
	union
	{
		struct IsoDirEntry dirEntry;
		BYTE padding[34];
	}root;

	BYTE volumeSetId[128],publisherId[128],dataPrepId[128],appId[128];
	BYTE copyrightFile[37],abstractFile[37],biblioFile[37];
	struct IsoDateTime createDate,modifyDate,expireDate,effectiveDate;
	BYTE reserved[1167];
}PACKED;

/* IsoFs specific vNode data */

struct IsoVNodePriv
{
	DWORD firstSector;
};

struct IsoSbPriv
{
	WORD sectorSize;
};

#define IsoGetPriv(vNode) ((struct IsoVNodePriv*)((vNode)->extraInfo))
#define IsoGetSbPriv(sb) ((struct IsoSbPriv*)((sb)->privData))

extern struct VNodeOps isoVNodeOps;
extern struct FileOps isoFileOps;

/* Library functions */
DWORD IsoConvertDate(char* date);

#endif
