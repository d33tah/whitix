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

#ifndef ELF_H
#define ELF_H

#include <typedefs.h>

#define PUSH_AUX_ENT(stackP,id,value) \
	do { \
	STACK_PUSH((stackP),(value)); \
	STACK_PUSH((stackP),(id)); \
	}while(0)

#define AT_NULL		0
#define AT_IGNORE	1
#define AT_EXECFD	2
#define AT_PHDR		3
#define AT_PHENT	4
#define AT_PHNUM	5
#define AT_PAGESZ	6
#define AT_BASE		7
#define AT_FLAGS	8
#define AT_ENTRY	9
#define AT_NOTELF	10
#define AT_UID		11
#define AT_EUID		12
#define AT_GID		13
#define AT_EGID		14

#define ELF_HEAD_CHECK(header) ((header)->elfMagic[0] == 0x7F && (header)->elfMagic[1] == 'E' && (header)->elfMagic[2] == 'L' \
		&& (header)->elfMagic[3] == 'F')

/* ELF file types */
#define ELF_REL		1
#define ELF_EXEC 	2
#define ELF_DYN 	3

/* ELF segment types */
#define ELF_TYPE_LOAD	1
#define ELF_TYPE_DYN	2
#define ELF_TYPE_INTERP	3

/* ELF permissions */
#define ELF_PERMS_RW	2

/* ElfCheckHeader - sanity checks the ELF header */
#define ElfCheckHeader(header, type) \
	(!ELF_HEAD_CHECK(header) || !((header)->fileType & (type)) || header->phEntrySize != sizeof(struct ElfSegmentHeader))

struct ElfHeader
{
	char elfMagic[4];
	BYTE bitness,endian,elfVer,reserved;
	DWORD reserved2[2];
	WORD fileType,machine;
	DWORD elfVer2;
	DWORD entryPoint;
	DWORD phOffset;
	DWORD shOffset;
	DWORD flags;
	WORD thisSize,phEntrySize;
	WORD phEntries,shEntrySize;
	WORD shEntries,strTabSectionIndex;
}PACKED;

struct ElfSegmentHeader
{
	DWORD type;
	DWORD fileOffset;
	DWORD virtualAddr;
	DWORD physAddr;
	DWORD fileSize;
	DWORD memSize;
	DWORD segFlags;
	DWORD segAlign;
}PACKED;

/* Section types. */
#define SEC_TYPE_SYMTAB		2
#define SEC_TYPE_STRTAB		3
#define SEC_TYPE_RELA		4
#define SEC_TYPE_NOBITS		8
#define SEC_TYPE_REL		9

/* Section flags. */
#define SEC_FLAGS_ALLOC		2
#define SEC_FLAGS_EXEC		4

struct ElfSectionHeader
{
	DWORD shName;
	DWORD shType;
	DWORD shFlags;
	DWORD shAddr;
	DWORD shOffset;
	DWORD shSize;
	DWORD shLink;
	DWORD shInfo;
	DWORD shAddrAlign;
	DWORD shEntSize;
}PACKED;

#define STN_UNDEF	0
#define STN_ABS		0xFFF1
#define STN_COMMON	0xFFF2

#define ELF_ST_TYPE(val) ((val) & 0xF)

#define STT_FUNC	2

struct ElfSymbol
{
	DWORD symName;
	DWORD symValue;
	DWORD symSize;
	BYTE symInfo;
	BYTE symOther;
	WORD symIndex;
}PACKED;

#define ELF_R_SYM(sym) ((sym) >> 8)
#define ELF_R_TYPE(sym) ((sym) & 0xFF)

/* Relocation types. */
#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GOT32     3
#define R_386_PLT32     4
#define R_386_COPY      5
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8
#define R_386_GOTOFF    9
#define R_386_GOTPC     10
#define R_386_NUM       11

struct ElfReloc
{
	DWORD addr;
	DWORD info;
}PACKED;

#endif
