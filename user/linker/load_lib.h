#ifndef LOAD_LIB_H
#define LOAD_LIB_H

#include "elf.h"

#include <syscalls.h>

struct ElfResolve* DlLoadElfLibrary(char* name);
void DlResolve();
struct ElfResolve* DlCreateResolveEntry(const char* name);

#define GOT_INIT(gotBase,module) \
do { \
	(gotBase)[1]=(unsigned long)module; \
	(gotBase)[2]=(unsigned long)DlResolve; \
}while(0)

#define ELF_HEAD_CHECK(header) ((header)->elfMagic[0] == 0x7F && (header)->elfMagic[1] == 'E' && (header)->elfMagic[2] == 'L' \
		&& (header)->elfMagic[3] == 'F')

#define ElfHeaderCheck(header,type) \
	(!ELF_HEAD_CHECK(header) || !((header)->fileType & (type)) || header->phEntrySize != sizeof(struct ElfProgHeader))

#endif
