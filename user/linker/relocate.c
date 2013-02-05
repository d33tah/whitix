#include "elf.h"
#include "lib.h"
#include "syscalls.h"
#include "hash.h"
#include "load_lib.h"

/* FIXME: Convert symbols to resolve entry before calling. */

Elf_Addr currAddr=0;

#ifndef DL
struct ElfResolve* loadedModules;
#endif

struct ElfSymResolve* symbolTables;

/* No-one needs to free, so this is a simple function */

void* Dl_malloc(int size)
{
	void* retVal;

	if (size > 4096)
		return 0;

	int offset=PAGE_OFFSET(currAddr);
	if (!currAddr || PAGE_SIZE-offset < size)
		/* Allocate a new page. */
		currAddr=SysMemoryMap(0, 4096, 7, -1, 0, 0);

	if (!currAddr)
	{
		link_puts("Could not allocate memory");
		SysExit(0);
	}

	retVal=(void*)currAddr;
	currAddr+=size;

	return retVal;
}

void DlFini()
{
	link_puts("Finishing!\n");
}

unsigned long DlResolver(struct ElfResolve* resolve,int relocEnt)
{
	unsigned long relAddr=resolve->dynamicInfo[DT_JMPREL];
	struct ElfReloc* reloc=(struct ElfReloc*)(relAddr+relocEnt);
	int symTabIdx=ELF_R_SYM(reloc->info);

	struct ElfSym* symTab=(struct ElfSym*)(resolve->dynamicInfo[DT_SYMTAB]);
	char* strTab=(char*)resolve->dynamicInfo[DT_STRTAB];
	char* symName=strTab+symTab[symTabIdx].name;
	char** gotAddr;

	unsigned long instrAddr=(unsigned long)(resolve->loadAddr+reloc->addr);
	gotAddr=(char**)instrAddr;

	unsigned long newAddr=DlFindHash(symName, ELF_RTYPE_CLASS_PLT);
	
	if (!newAddr)
	{
		link_puts("Can't resolve");
		link_puts(symName);
		link_puts("in");
		link_puts(resolve->libName);
		link_puts("\n");
		SysExit(0);
	}

	*gotAddr=(char*)newAddr;
	return newAddr;
}

typedef int (*DlParserFunc)(struct ElfResolve*,struct ElfReloc*,struct ElfSym*,char*);

/***********************************************************************
 *
 * FUNCTION: DlParseReloc
 *
 * DESCRIPTION: Parse a relocation table.
 *
 * PARAMETERS: symbols - link to a mapped in symbol table structure.
 *			   relAddr - address of the relocation table.
 *			   relSize - size of the relocation table.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void DlParseReloc(struct ElfSymResolve* symbols,unsigned long relAddr,unsigned long relSize,DlParserFunc parserFunc)
{
	unsigned int i;
	struct ElfResolve* resolve=symbols->resolveEntry;
	struct ElfReloc* relocAddr=(struct ElfReloc*)relAddr;
	struct ElfSym* symTable=(struct ElfSym*)(resolve->dynamicInfo[DT_SYMTAB]);
	char* strTab=(char*)(resolve->dynamicInfo[DT_STRTAB]);

	/* Measure the size in entries rather than bytes. */
	relSize/=sizeof(struct ElfReloc);

	/* 
	 * Iterate through the whole relocation table, and see if relocations 
	 * are needed. 
	 */

	for (i=0; i<relSize; i++,relocAddr++)
	{
		if ((parserFunc(resolve,relocAddr,symTable,strTab)))
		{
			/* Could not relocate the symbol. Unlikely to happen. */
			link_puts("Could not relocate");
			SysExit(0);
		}
	}
}

/***********************************************************************
 *
 * FUNCTION: DlDoLazyReloc
 *
 * DESCRIPTION: Lazily relocate.
 *
 * PARAMETERS: resolve - entry of module being relocated.
 *			   currReloc - the relocation to resolve.
 *			   symTab - symbol table. Not used.
 *			   strTab - string table. Not used.
 *
 * RETURNS: status, 0 if success, 1 if failure.
 *
 ***********************************************************************/

int DlDoLazyReloc(struct ElfResolve* resolve,struct ElfReloc* currReloc,struct ElfSym* symTab,char* strTab)
{
	unsigned long* relAddr=(unsigned long*)(resolve->loadAddr+currReloc->addr);

	switch (ELF_R_TYPE(currReloc->info))
	{
	case R_386_NONE:
		break;

	case R_386_JMP_SLOT:
		*relAddr+=resolve->loadAddr;
		break;

	default:
		return 1;
	}

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: DlDoReloc
 *
 * DESCRIPTION: Relocate a relocation.
 *
 * PARAMETERS: resolve - entry of module being relocated.
 *			   currReloc - the relocation to resolve.
 *			   symTab - address of the symbol table.
 *			   strTab - address of the string table.
 *
 * RETURNS: status, 0 if success, 1 if failure.
 *
 ***********************************************************************/

int DlDoReloc(struct ElfResolve* resolve,struct ElfReloc* currReloc,struct ElfSym* symTab,char* strTab)
{
	int symTabIdx=ELF_R_SYM(currReloc->info);
	char* symName=strTab+symTab[symTabIdx].name;
	unsigned long symbolAddr=0;
	unsigned long* relAddr=(unsigned long*)(resolve->loadAddr+currReloc->addr);
	
	if (symTabIdx)
	{
		symbolAddr=DlFindHash(symName, ELF_TO_RTYPE_CLASS(ELF_R_TYPE(currReloc->info)));
		if (!symbolAddr && ELF_ST_BIND(symTab[symTabIdx].info) != STB_WEAK)
		{
			link_puts("Could not find symbol ");
			link_puts(symName);
			link_puts("\n");
//			SysExit(0);
		}
	}

	switch (ELF_R_TYPE(currReloc->info))
	{
		case R_386_NONE:
			break;

		case R_386_32:
			*relAddr+=symbolAddr;
			break;

		case R_386_PC32:
			*relAddr+=symbolAddr-(unsigned long)relAddr;
			break;

		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT:
			*relAddr=symbolAddr;
			break;

		case R_386_RELATIVE:
			*relAddr+=resolve->loadAddr;
			break;

		case R_386_COPY:
			if (symbolAddr)
				link_memcpy((char*)relAddr,(char*)symbolAddr,symTab[symTabIdx].size);
			break;

//		default:
//			link_printhex("Could not relocate type. info = ", currReloc->info);
//			return 1;
	}

	return 0;
}

int DlDoTlsReloc(struct ElfResolve* resolve,struct ElfReloc* currReloc,struct ElfSym* symTab,char* strTab)
{
	int symTabIdx=ELF_R_SYM(currReloc->info);
	unsigned long* relAddr=(unsigned long*)(resolve->loadAddr+currReloc->addr);
	
	switch (ELF_R_TYPE(currReloc->info))
	{				
		case R_386_TLS_DTPMOD32:
			*relAddr = resolve->tlsModId;
			break;
			
		case R_386_TLS_DTPOFF32:
			*relAddr = symTab[symTabIdx].value; /* Check */
			break;
		
		case 14 ... 34:
		case 37 ... 38:
			link_puts("TLS\n");
			break;
	}
	
	return 0;
}

void DlFixupSymbols(struct ElfSymResolve* symbols)
{
	unsigned long relocAddr,relCount,relocSize;
	struct ElfResolve* resolve=symbols->resolveEntry;

	if (symbols->next)
		DlFixupSymbols(symbols->next); /* Got plenty of stack space. */

	relocSize=resolve->dynamicInfo[DT_RELSZ];

	/* May need to make text rewriteable before relocation */
	if (resolve->dynamicInfo[DT_TEXTREL])
		SysMemoryProtect(resolve->textAddr, resolve->textLength, 3);

	if (resolve->dynamicInfo[DT_REL] && !(resolve->initFlags & INIT_RELOCS_DONE))
	{
		relocAddr=resolve->dynamicInfo[DT_REL];
		relCount=resolve->dynamicInfo[DT_RELCOUNT_IDX];

		if (relCount)
		{
			relocSize-=relCount*sizeof(struct ElfReloc);
			ElfRelative(resolve->loadAddr, relocAddr, relCount);
			relocAddr+=relCount*sizeof(struct ElfReloc);
		}

		DlParseReloc(symbols, relocAddr, relocSize, DlDoReloc);
		resolve->initFlags |= INIT_RELOCS_DONE;
	}
	
	if (resolve->tlsInitImage && !(resolve->initFlags & INIT_TLS_RELOCS_DONE))
	{
		DlParseReloc(symbols, relocAddr, relocSize, DlDoTlsReloc);
		resolve->initFlags |= INIT_TLS_RELOCS_DONE;
	}

	if (resolve->dynamicInfo[DT_JMPREL] && !(resolve->initFlags & INIT_JMP_RELOCS_DONE))
	{
		DlParseReloc(symbols,resolve->dynamicInfo[DT_JMPREL],
			resolve->dynamicInfo[DT_PLTRELSZ], DlDoLazyReloc);
		resolve->initFlags |= INIT_JMP_RELOCS_DONE;
	}
	
	/* May need to make text read-only again. */
	if (resolve->dynamicInfo[DT_TEXTREL])
		SysMemoryProtect(resolve->textAddr, resolve->textLength, 1);
}

int DlRelocate(struct ElfAuxEnt* auxEntries, struct ElfResolve* resolve, char** argv)
{
	struct ElfProgHeader* header=(struct ElfProgHeader*)auxEntries[AT_PHDR].value;
	int i;
	Elf_Addr loadAddr=0;
	unsigned long phNum=auxEntries[AT_PHNUM].value;
	char* progName=argv[0];
	struct ElfResolve* entry;

	/* Find the runtime load address of the main executable, to see if it's position independent. */
	for (i=0; i<phNum; i++)
		if (header[i].type == PT_PHDR)
		{
			loadAddr=(Elf_Addr)(auxEntries[AT_PHDR].value-header[i].vAddr);
			break;
		}

	if (loadAddr)
		link_puts("Position-independent executable");

	/* Look for dynamic section(s) in the main executable. */
	for (i=0; i<phNum; i++)
	{
		if (header[i].type == PT_DYNAMIC)
		{
			struct ElfDyn* dynEntries=(struct ElfDyn*)header[i].vAddr;
			unsigned long* pltGot;
			struct ElfResolve* entry;
			
			entry = DlCreateResolveEntry(progName);
			
			ElfParseDynInfo(dynEntries, entry->dynamicInfo, loadAddr);
			entry->pHeaders=(struct ElfProgHeader*)auxEntries[AT_PHDR].value;
			entry->phNum = auxEntries[AT_PHNUM].value;
			entry->dynamicAddr = dynEntries;
			entry->type = ELF_EXECUTABLE; /* TODO: Check header? */
			
			pltGot=(unsigned long*)(entry->dynamicInfo[DT_PLTGOT]);
			
			if (pltGot)
				GOT_INIT(pltGot, entry);

			/* Do some TLS setup for the main executable. */
			entry->tlsModId = DlTlsNewId();

			/* Allocate symbol table */
			struct ElfSymResolve* symEnt=symbolTables=(struct ElfSymResolve*)Dl_malloc(sizeof(struct ElfSymResolve));
			symEnt->resolveEntry=entry;
			
			DlAddResolveEntry(entry);
		}
	}
	
	/* Add the interpreter to the resolve chain. */
	DlAddResolveEntry(resolve);
	resolve->initFlags=INIT_RELOCS_DONE;

	if (!symbolTables)
	{
		link_puts("Main executable has no dynamic section\n");
		SysExit(0);
	}

	for (entry=loadedModules; entry; entry=entry->next)
	{
		struct ElfDyn* dynEntry;
		
		for (dynEntry=(struct ElfDyn*)entry->dynamicAddr; dynEntry->tag; dynEntry++)
		{
			if (dynEntry->tag != DT_NEEDED)
				continue;
				
			/* Load libraries that are needed by the application. */
			struct ElfResolve* newEnt;
			char* name=(char*)(entry->dynamicInfo[DT_STRTAB] + dynEntry->val);
				
			newEnt=DlLoadElfLibrary(name);

			if (!newEnt)
			{
				link_puts("Failed to load:");
				link_puts(name);
				SysExit(0);
			}
		}
	}

	if (symbolTables)
		DlFixupSymbols(symbolTables);

	return 0;
}
