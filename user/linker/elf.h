/* TODO: Needs expansion (more defines) and a cleanup. */

#ifndef ELF_H
#define ELF_H

#include "syscalls.h"

#define _ELF_NATIVE_CLASS		32

#define INIT_RELOCS_DONE 		1
#define INIT_JMP_RELOCS_DONE	2
#define INIT_TLS_RELOCS_DONE	4

typedef unsigned long 	Elf_Addr;
typedef unsigned long 	Elf_Word;
typedef unsigned short 	Elf_Half;
typedef unsigned long	Elf_Offset;

#define ELF_NIDENT	16

/* Values for ElfHeader.fileType */
#define ELF_TYPE_NONE	0
#define ELF_TYPE_REL	1
#define ELF_TYPE_LOAD	2
#define ELF_TYPE_DYN	3

struct ElfHeader
{
	unsigned char 	elfMagic[ELF_NIDENT];
	Elf_Half	fileType;
	Elf_Half	machine;
	Elf_Word	version;
	Elf_Addr	entry;
	Elf_Offset	phOffset;
	Elf_Offset	shOffset;
	Elf_Word	flags;
	Elf_Half	ehSize;
	Elf_Half	phEntrySize;
	Elf_Half	phEntries;
	Elf_Half	shEntrySize;
	Elf_Half	shEntries;
	Elf_Half	shStrIndex;
};

/* Values for ElfProgHeader.type */
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_TLS		7
#define PT_NUM		8

#define PROT_EXEC	1
#define PROT_WRITE	2
#define PROT_READ	4

struct ElfProgHeader
{
	Elf_Word	type;
	Elf_Offset	fileOffset;
	Elf_Addr	vAddr;
	Elf_Addr	pAddr;
	Elf_Word	fileSize;
	Elf_Word	memSize;
	Elf_Word	segFlags;
	Elf_Word	align;
}__attribute((packed));

#define DT_NULL		0		/* Marks end of dynamic section */
#define DT_NEEDED	1		/* Name of needed library */
#define DT_PLTRELSZ	2		/* Size in bytes of PLT relocs */
#define DT_PLTGOT	3		/* Processor defined value */
#define DT_HASH		4		/* Address of symbol hash table */
#define DT_STRTAB	5		/* Address of string table */
#define DT_SYMTAB	6		/* Address of symbol table */
#define DT_RELA		7		/* Address of Rela relocs */
#define DT_RELASZ	8		/* Total size of Rela relocs */
#define DT_RELAENT	9		/* Size of one Rela reloc */
#define DT_STRSZ	10		/* Size of string table */
#define DT_SYMENT	11		/* Size of one symbol table entry */
#define DT_INIT		12		/* Address of init function */
#define DT_FINI		13		/* Address of termination function */
#define DT_SONAME	14		/* Name of shared object */
#define DT_RPATH	15		/* Library search path (deprecated) */
#define DT_SYMBOLIC	16		/* Start symbol search here */
#define DT_REL		17		/* Address of Rel relocs */
#define DT_RELSZ	18		/* Total size of Rel relocs */
#define DT_RELENT	19		/* Size of one Rel reloc */
#define DT_PLTREL	20		/* Type of reloc in PLT */
#define DT_DEBUG	21		/* For debugging; unspecified */
#define DT_TEXTREL	22		/* Reloc might modify .text */
#define DT_JMPREL	23		/* Address of PLT relocs */
#define	DT_BIND_NOW	24		/* Process relocations of object */
#define	DT_INIT_ARRAY	25		/* Array with addresses of init fct */
#define	DT_FINI_ARRAY	26		/* Array with addresses of fini fct */
#define	DT_INIT_ARRAYSZ	27		/* Size in bytes of DT_INIT_ARRAY */
#define	DT_FINI_ARRAYSZ	28		/* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH	29		/* Library search path */
#define DT_FLAGS	30		/* Flags for the object being loaded */
#define DT_ENCODING	32		/* Start of encoded range */
#define DT_PREINIT_ARRAY 32		/* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ 33		/* size in bytes of DT_PREINIT_ARRAY */
#define	DT_NUM		34		/* Number used */
#define DT_RELCOUNT_IDX	DT_NUM
#define DT_GNU_HASH_IDX	DT_NUM + 1
#define DT_TOTAL DT_GNU_HASH_IDX+1

#define	DT_LOOS		0x60000000	/* Operating system specific range */
#define DT_GNU_HASH	0x6ffffef5
#define DT_VERSYM	0x6ffffff0	/* Symbol versions */
#define DT_RELCOUNT	0x6ffffffa
#define DT_VERDEF	0x6ffffffc	/* Versions defined by file */
#define DT_VERDEFNUM	0x6ffffffd	/* Number of versions defined by file */
#define DT_VERNEED	0x6ffffffe	/* Versions needed by file */
#define DT_VERNEEDNUM	0x6fffffff	/* Number of versions needed by file */
#define	DT_HIOS		0x6fffffff
#define	DT_LOPROC	0x70000000	/* Processor-specific range */
#define	DT_HIPROC	0x7fffffff

struct ElfSymResolve
{
	struct ElfResolve* resolveEntry;
	struct ElfSymResolve* prev,*next;
};

#define ELF_EXECUTABLE		0
#define ELF_LIBRARY			1

struct ElfResolve
{
	struct ElfResolve* next,*prev;
	char* libName;
	int type, picLib; /* Library or executable? */
	unsigned long loadAddr;
	int phNum;
	
	/* Hash tables. */
	unsigned long numBuckets, gnuMaskIdxBits, gnuShift, gnuBitMask;
		
	union
	{
		const unsigned long* gnuBuckets;
		const unsigned long* elfChains;
	};
	
	union
	{
		unsigned long* elfBuckets;
		unsigned long* gnuChainZero;
	};
	
	/* Headers */
	struct ElfHeader fileHeader;
	struct ElfProgHeader* pHeaders;
	
	/* Addresses of special sections */
	unsigned long textAddr, textLength;
	unsigned long dynamicAddr;
	
	/* Thread local storage */
	unsigned long tlsInitImage, tlsSize;
	unsigned long tlsModId;
	
	int refs,initFlags;
	unsigned long dynamicInfo[DT_TOTAL];
};

#define AT_NULL		0		/* End of vector */
#define AT_IGNORE	1		/* Entry should be ignored */
#define AT_EXECFD	2		/* File descriptor of program */
#define AT_PHDR		3		/* Program headers for program */
#define AT_PHENT	4		/* Size of program header entry */
#define AT_PHNUM	5		/* Number of program headers */
#define AT_PAGESZ	6		/* System page size */
#define AT_BASE		7		/* Base address of interpreter */
#define AT_FLAGS	8		/* Flags */
#define AT_ENTRY	9		/* Entry point of program */
#define AT_NOTELF	10		/* Program is not ELF */
#define AT_UID		11		/* Real uid */
#define AT_EUID		12		/* Effective uid */
#define AT_GID		13		/* Real gid */
#define AT_EGID		14		/* Effective gid */
#define AT_CLKTCK	17		/* Frequency of times() */

struct ElfAuxEnt
{
	int id;
	int value;
};

struct ElfDyn
{
	unsigned long tag;
	Elf_Word val;
};

#define ELF_R_SYM(sym) ((sym) >> 8)
#define ELF_R_TYPE(sym) ((sym) & 0xFF)

#define ELF_RTYPE_CLASS_PLT			1
#define ELF_RTYPE_CLASS_COPY		2

#define ELF_TO_RTYPE_CLASS(type) \
	((( type == R_386_JMP_SLOT) * ELF_RTYPE_CLASS_PLT) \
	| (( type == R_386_COPY) * ELF_RTYPE_CLASS_COPY))

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
//#define R_386_NUM       11
#define R_386_TLS_DTPMOD32 35
#define R_386_TLS_DTPOFF32 36

struct ElfSym
{
	unsigned long name;
	unsigned long value;
	unsigned long size;
	unsigned char info;
	unsigned char symOther;
	unsigned short shIndex;
};

typedef unsigned long ElfSymIndex;

#define ELF_ST_BIND(val) (((unsigned int)(val)) >> 4)
#define ELF_ST_TYPE(val) ((val) & 0xF)

#define STN_UNDEF	0

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2

#define STT_FUNC	2
#define STT_COMMON	3
#define STT_TLS		6

#define SHN_UNDEF	0

struct ElfReloc
{
	Elf_Addr addr;
	Elf_Word info;
};

#endif
