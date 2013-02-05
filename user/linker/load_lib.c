#include "load_lib.h"
#include "hash.h"
#include "lib.h"
#include "elf.h"
#include "relocate.h"

#include <errno.h>
int dlErrno=0;

char* searchPaths[]=
{
	"/System/Runtime/",
	"",
};

void DlAddFilePath(char* out, char* directory, char* name)
{
	/* Prepend a path to the filename. */
	link_strcpy(out,directory);
	link_strcpy(out+link_strlen(directory),name);
}

/***********************************************************************
 *
 * FUNCTION: DlLoadHeaders
 *
 * DESCRIPTION: Load an ELF executable's headers from a file descriptor.
 *
 * PARAMETERS: name - name of the library to be loaded.
 *			   fileHeader - main ELF header. Return variable.
 *			   progHeaders - program header array. Dynamically allocated
 *							and read in to.
 *
 * RETURNS: 0 if success, < 0 if failure, file descriptor if > 0.
 *
 ***********************************************************************/

int DlLoadHeaders(char* name, struct ElfResolve* entry)
{
	int	result,fd;
	struct ElfHeader* fileHeader=&entry->fileHeader;

	/* Open the library to read the headers and map in the sections later. */
	fd=SysOpen(name, _SYS_FILE_READ, 0);
	if (fd < 0)
		return fd;

	/* Seek to the beginning of the file and read the file header. */
	SysSeek(fd,0,0);
	result=SysRead(fd, fileHeader, sizeof(struct ElfHeader));

	if (result < 0)
		return result;

	/* Checks the ELF magic number, executable type and other sanity checks. */
	if (ElfHeaderCheck(fileHeader, ELF_TYPE_DYN))
		return -1;

	/* Seek to the program header(s) file offset and read 'phEntries' number of entries,
	   dynamically allocating that number first. */
	entry->pHeaders = (struct ElfProgHeader*)
		Dl_malloc(sizeof(struct ElfProgHeader) * fileHeader->phEntries);
	SysSeek(fd, fileHeader->phOffset, 0);
	result=SysRead(fd, entry->pHeaders, sizeof(struct ElfProgHeader) * fileHeader->phEntries);

	if (result < 0)
		return result;

	entry->phNum = fileHeader->phEntries;

	return fd;
}

int DlDoLoad(struct ElfResolve* entry)
{
	char fileName[512];
	int i, err;

	/* Search through library paths for the file. */
	for (i=0; i<(sizeof(searchPaths)/sizeof(searchPaths[0])); i++)
	{
		DlAddFilePath(fileName, searchPaths[i], entry->libName);
		
		err=DlLoadHeaders(fileName, entry);
		
		if (err > 0)
			return err;
	}

	return err;
}

struct ElfResolve* DlCheckLoadedModules(const char* name)
{
	struct ElfResolve* entry;
	
	/* TODO: Names may clash, so stat the filename and compare ids? */
	for (entry=loadedModules; entry; entry=entry->next)
	{
		if (!link_strcmp(entry->libName,name))
		{
			entry->refs++;
			return entry;
		}
	}
	
	return NULL;
}

int DlGetAddresses(struct ElfResolve* entry)
{
	int i;
	
	/* Assign the file type. TODO: Check header */
	entry->type = ELF_LIBRARY;
	entry->picLib = 1;
	
	for (i=0; i<entry->fileHeader.phEntries; i++)
	{
		struct ElfProgHeader* pHeader = &entry->pHeaders[i];
		
		switch (pHeader->type)
		{
			case PT_DYNAMIC:
				if (entry->dynamicAddr)
					continue;
					
				/* The dynamic section holds the ELF file's dynamic information address. */
				entry->dynamicAddr = pHeader->vAddr;
				break;
				
			case PT_LOAD:
				/* Is it a position independent library if it has such a high
				 * load address? Most likely not. */
				if (pHeader->vAddr > 0x1000000 && !i)
					entry->picLib = 0;
				
				if (pHeader->segFlags == (PROT_EXEC | PROT_READ))
				{
					entry->textAddr = pHeader->vAddr;
					entry->textLength = pHeader->memSize;
				}
				
				break;
				
			case PT_TLS:
				
				if (pHeader->memSize == 0)
					continue;
					
				entry->tlsSize = pHeader->memSize;
				entry->tlsInitImage = pHeader->vAddr;
				
				if (entry->type == ELF_LIBRARY)
				{
					entry->tlsModId = DlTlsNewId();
					break;
				}
				
				link_puts("TODO\n");
				SysExit(0);
					
				break;
		}
	}
	
	return 0;
}

int DlMapLibrarySections(int fd, struct ElfResolve* entry)
{
	Elf_Addr elfBss=0, mapAddr=0;
	int i;

	for (i=0; i<entry->fileHeader.phEntries; i++)
	{
		struct ElfProgHeader* pHeader = &entry->pHeaders[i];
	
		if (pHeader->type != PT_LOAD)
			continue;
			
		/* Only deal with loadable sections */
	
		/* Set permissions. */
		int mmapFlags=_SYS_MMAP_PRIVATE;
		int elfProt=1; /* TODO: Document ... */
			
		elfProt|= (pHeader->segFlags & 2);

		if (entry->loadAddr || !entry->picLib)
			mmapFlags |= _SYS_MMAP_FIXED;

		/*
		 * loadAddr will be zero on the first iteration, so we can
		 * find out where the section has been mapped to, if it is a
		 * PIC library.
		 */
		pHeader->vAddr += entry->loadAddr;

		mapAddr=SysMemoryMap(PAGE_ALIGN(pHeader->vAddr), 					  	/* Start address. */
							 PAGE_ALIGN_UP(pHeader->fileSize+PAGE_OFFSET(pHeader->vAddr)), /* Size */
							 elfProt,											  	/* Protection */
							 fd,												  	/* Backing file */
							 pHeader->fileOffset-PAGE_OFFSET(pHeader->vAddr),  /* File offset */
							 mmapFlags);									/* Flags */

		if (!mapAddr)
		{
			link_puts("Failed to map section of shared library\n");
			return -1;
		}

		if (!entry->loadAddr)
			entry->loadAddr=mapAddr-PAGE_OFFSET(pHeader->vAddr);

		if (elfBss < pHeader->vAddr+pHeader->fileSize)
			elfBss=pHeader->vAddr+pHeader->fileSize;

		/* Map in the BSS */
		if (pHeader->memSize > pHeader->fileSize)
		{
			unsigned long bssSize=PAGE_ALIGN(pHeader->memSize-pHeader->fileSize);
			unsigned long pageAddr=PAGE_ALIGN_UP(pHeader->vAddr+pHeader->fileSize);
				
			if (bssSize)
				SysMemoryMap(pageAddr, bssSize, 3, -1, 0, _SYS_MMAP_PRIVATE | _SYS_MMAP_FIXED);
				
			/* Zero out the bss. */
			if (PAGE_OFFSET(elfBss) > 0)
				link_memset(elfBss, 0, PAGE_SIZE-PAGE_OFFSET(elfBss));
		}
	}
	
	/* Update various addresses with the new load address as the base. */
	if (entry->picLib)
	{
		entry->dynamicAddr += entry->loadAddr;
		entry->textAddr += entry->loadAddr;
	}

	return 0;
}

void DlUpdateSymbolTables(struct ElfResolve* entry)
{
	struct ElfSymResolve* symEntry=symbolTables;

	if (symbolTables)
	{
		/* Get to the end of the list ...*/
		while (symEntry->next)
			symEntry=symEntry->next;
	
		/* and add it there. */
		symEntry->next=(struct ElfSymResolve*)Dl_malloc(sizeof(struct ElfSymResolve));
		symEntry->next->prev=symEntry;
		symEntry=symEntry->next;
		symEntry->resolveEntry=entry;
	}else{
		symbolTables=(struct ElfSymResolve*)Dl_malloc(sizeof(struct ElfSymResolve));
		symbolTables->next=NULL;
		symbolTables->resolveEntry=entry;
	}
}

struct ElfResolve* DlCreateResolveEntry(const char* name)
{
	struct ElfResolve* entry = (struct ElfResolve*)Dl_malloc(sizeof(struct ElfResolve));
	
	entry->libName = link_strdup(name);
	
	return entry;
}

void DlAddResolveEntry(struct ElfResolve* entry)
{
	DlAddHashTable(entry);
	DlUpdateSymbolTables(entry);
}

struct ElfResolve* DlLoadElfLibrary(char* name)
{
	int fd;
	struct ElfResolve* entry;

	/* Check if we've loaded this library into memory already. */
	entry=DlCheckLoadedModules(name);
	
	if (entry)
		return entry;

	entry = DlCreateResolveEntry(name);

	/* Get the library's headers. */
	fd=DlDoLoad(entry);

	if (fd < 0)
	{
		dlErrno=ENOENT;
		return NULL;
	}

	/* Get the library's dynamic information address. We also figure out whether the library
	 * has position independent code from here.
	 */
	DlGetAddresses(entry);
	
	if (!entry->dynamicAddr)
	{
		link_puts("Library missing a dynamic section");
		dlErrno=EINVAL;
		return NULL;
	}
	
	/* Now map in the sections themselves. */
	if (DlMapLibrarySections(fd, entry))
		return NULL;

	if (!entry->loadAddr)
		return NULL;

	/* Close the file descriptor we used for mapping the sections in. */
	SysClose(fd);

	/* Parse dynamic entries. Reads the contents at dynamicAddr into dynEntries. */
	ElfParseDynInfo((struct ElfDyn*)entry->dynamicAddr, entry->dynamicInfo, entry->loadAddr);

	DlAddResolveEntry(entry);

	/* Initialize the global offset table (GOT) */
	unsigned long* pltPointer=(unsigned long*)(entry->dynamicInfo[DT_PLTGOT]);
	if (pltPointer)
		GOT_INIT(pltPointer, entry);

	return entry;
}
