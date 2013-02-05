#ifndef LIB_H
#define LIB_H

#include <syscalls.h>
#include "elf.h"

void* Dl_malloc(int size);

/* Whole code probably i386 specific */

#define STDOUT	0
#define STDIN	1
#define STDERR	2

#define NULL (void*)0

/* The dynamic table address is contained in the first entry of the GOT */
static inline Elf_Addr ElfGetDynamic()
{
	register Elf_Addr* got asm("%ebx");
	return *got;
}

static inline Elf_Addr GetElfLoadAddress()
{
	extern int dummy; /* Replace with real global soon. */
	Elf_Addr addr;
	asm volatile("leal _start@GOTOFF(%%ebx),%0\n"
				 "subl _start@GOT(%%ebx),%0"
				 : "=r"(addr) : "m"(dummy) : "cc");
	return addr;
}

static inline int link_strlen(char* str)
{
	int i=0;
	while (*str++) i++;
	return i;
}

static inline void link_memcpy(void* dest,void* src,int size)
{
	char* d=(char*)dest;
	char* s=(char*)src;
	
	while (size--)
		*d++=*s++;
}

static inline void link_memset(void* dest,char value,int size)
{
	char* iterate=(char*)dest;

	while (size--)
		*iterate++=value;
}

static inline void link_puts(char* str)
{
	SysWrite(STDOUT,str,link_strlen(str));
}

static inline char* link_strdup(char* str)
{
	int len=link_strlen(str)+1;

	char* s=(char*)Dl_malloc(len);
	link_memcpy(s,str,len);
	return s;
}

static inline void link_strcpy(char* dest,char* src)
{
	while (*src)
		*dest++=*src++;

	*dest='\0';
}

static inline int link_strcmp(const char* s1,const char* s2)
{
	unsigned char res=0;

   	 while (1)
		if ((res=*s1-*s2++) || !*s1++)
			break;

	return res;
}

static inline void link_printhex(char* s,unsigned long num)
{
	int i;
	char c[2*sizeof(unsigned long)+1];
	char hexDigits[]="0123456789ABCDEF";

	for (i=(2*sizeof(unsigned long))-1; i>=0; i--)
		c[(2*sizeof(unsigned long))-1-i]=hexDigits[((num >> (i*4)) & 0xF)];

	c[2*sizeof(unsigned long)]=0;

	if (s)
		SysWrite(STDOUT,(unsigned char*)s,link_strlen(s));
	link_puts(c);
}

#define ADJUST_DYN_INFO(tag) \
	do { \
		if (dynamicInfo[tag]) \
			dynamicInfo[tag]+=loadAddr; \
	}while(0);

static inline void ElfParseDynInfo(struct ElfDyn* dynEntry, unsigned long dynamicInfo[DT_TOTAL], Elf_Addr loadAddr)
{
	/* Clear the dynamic info structure */
	link_memset(dynamicInfo, 0,(DT_TOTAL)*4);

	for (; dynEntry->tag; dynEntry++)
	{
		if (dynEntry->tag < DT_NUM)
		{
			dynamicInfo[dynEntry->tag]=dynEntry->val;

			if (dynEntry->tag == DT_TEXTREL)
				dynamicInfo[DT_TEXTREL]=1;
		}else if (dynEntry->tag == DT_RELCOUNT)
			dynamicInfo[DT_RELCOUNT_IDX]=dynEntry->val;
		else if (dynEntry->tag == DT_GNU_HASH)
			dynamicInfo[DT_GNU_HASH_IDX]=dynEntry->val;

		/* Other things like DT_FLAGS here. */
	}

	/* Adjust for load address. */
	ADJUST_DYN_INFO(DT_HASH);
	ADJUST_DYN_INFO(DT_PLTGOT);
	ADJUST_DYN_INFO(DT_STRTAB);
	ADJUST_DYN_INFO(DT_SYMTAB);
	ADJUST_DYN_INFO(DT_REL);
	ADJUST_DYN_INFO(DT_JMPREL);

	if (dynamicInfo[DT_GNU_HASH_IDX])
		ADJUST_DYN_INFO(DT_GNU_HASH_IDX);
}

static inline void ElfRelative(unsigned long loadOff,
	unsigned long relAddr, unsigned long relCount)
{
	struct ElfReloc* reloc=(void*)relAddr;
	--reloc;
	do
	{
		Elf_Addr* relocAddr=(void*)(loadOff+(++reloc)->addr);
		*relocAddr+=loadOff;
	}while (--relCount);
}

#endif
