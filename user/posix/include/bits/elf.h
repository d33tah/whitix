#ifndef __POSIX_ELF_H
#define __POSIX_ELF_H

#define ElfW(sym) Elf32_##sym

typedef unsigned long Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned long Elf32_Off;
typedef signed long Elf32_Sword;
typedef unsigned long Elf32_Word;

#define EI_NIDENT	16

/* Structure is a standard */

typedef struct
{
	unsigned char e_ident[EI_NIDENT];
	ElfW(Half) e_type, e_machine;
	ElfW(Word) e_version;
	ElfW(Addr) e_entry;
	ElfW(Off) e_phoff,e_shoff;
	ElfW(Word) e_flags;
	ElfW(Half) e_ehsize,e_phentsize;
	ElfW(Half) e_phnum,e_shentsize;
	ElfW(Half) e_shnum,e_shstrndx;
}Elf32_Ehdr;

/* Header types */

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6

/* i386 relocations */

#define R_386_NONE		0
#define R_386_32		1
#define R_386_PC32		2
#define R_386_GOT32		3
#define R_386_PLT32		4
#define R_386_COPY		5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC		10
#define R_386_NUM		11

#define PF_W	2

typedef struct
{
	ElfW(Word) p_type;
	ElfW(Off) p_offset;
	ElfW(Addr) p_vaddr,p_paddr;
	ElfW(Word) p_filesz,p_memsz,p_flags,p_align;
}Elf32_Phdr;

typedef struct
{
	ElfW(Sword) d_tag;
	union
	{
		ElfW(Sword) d_val;
		ElfW(Addr) d_ptr;
	} d_un;
}Elf32_Dyn;

/* Dynamic section ids */

#define DT_NULL			0
#define DT_NEEDED		1
#define DT_PLTRELSZ		2
#define DT_PLTGOT		3
#define DT_HASH			4
#define DT_STRTAB		5
#define DT_SYMTAB		6
#define DT_RELA			7
#define DT_RELASZ		8
#define DT_RELAENT		9
#define DT_STRSZ		10
#define DT_SYMENT		11
#define DT_INIT			12
#define DT_FINI			13
#define DT_SONAME		14
#define DT_RPATH		15
#define DT_SYMBOLIC		16
#define DT_REL			17
#define DT_RELSZ		18
#define DT_RELENT		19
#define DT_PLTREL		20
#define DT_DEBUG		21
#define DT_TEXTREL		22
#define DT_JMPREL		23
#define DT_BIND_NOW		24
#define DT_INIT_ARRAY	25
#define DT_FINI_ARRAY	26
#define DT_INIT_ARRAYSZ	27
#define DT_FINI_ARRAYSZ	28

/* Not strictly, copied to here however */
#define DT_RELCOUNT_IDX 29
#define DT_RELCOUNT 0x6FFFFFFA

#define DT_NUM	30

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

#define ELF_TYPE_LOAD	1
#define ELF_TYPE_DYN	2

#define STN_UNDEF 0

#define SHN_UNDEF 0

/* Symbol defines */

#define ELF_ST_BIND(x) ((x) >> 4)
#define ELF_ST_TYPE(x) ((x) & 0xF)

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_NUM		3

#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_COMMON	5
#define STT_TLS		6
#define STT_NUM		7

#define STT_LOPROC	13
#define STT_HIPROC	15

struct ElfAuxEnt
{
	unsigned long id,value;
};

struct ElfReloc
{
	unsigned long addr,info;
};

struct ElfSym
{
	unsigned long name,value,size;
	unsigned char info,other;
	unsigned short shIndex;
};

struct r_debug
{
	int r_version;
	struct link_map* r_map;
	ElfW(Addr) r_brk;
	
	enum
	{
		RT_CONSISTENT,
		RT_ADD,
		RT_DELETE
	} r_state;
	
	ElfW(Addr) r_ldbase;
};

struct link_map
{
	ElfW(Addr) l_addr;
	char* l_name;
	ElfW(Dyn)* l_ld;
	struct link_map* l_next, *l_prev;
};

struct dl_phdr_info
{
	ElfW(Addr) dlpi_addr;
	const char* dlpi_name;
	const ElfW(Phdr)* dlpi_phdr;
	ElfW(Half) dlpi_phnum;
};

int dl_iterate_phdr( int (*callback) (struct dl_phdr_info* info, size_t size, void* data),
		void* data);

#define ELF_R_SYM(x) ((x) >> 8)
#define ELF_R_TYPE(x) ((unsigned char)((x) & 0xFF))

typedef unsigned long ElfSymIndex;

#endif /* __POSIX_ELF_H */
