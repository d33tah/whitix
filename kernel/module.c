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

#include <config.h>
#include <console.h>
#include <elf.h>
#include <error.h>
#include <malloc.h>
#include <module.h>
#include <i386/i386.h>
#include <i386/virtual.h>
#include <sections.h>
#include <slab.h>
#include <sys.h>
#include <task.h>
#include <print.h>
#include <panic.h>
#include <keobject.h>

LIST_HEAD(moduleList);
Spinlock moduleListLock;

void ModuleRelease(struct KeObject* object);
KE_OBJECT_TYPE(moduleType, ModuleRelease, NULL, OffsetOf(struct Module, object));

struct KeSet moduleSet;

void* ModuleSymbolFind(struct Module* module, const char* symName, int type)
{
	DWORD i;
	struct ElfSymbol* symbol;

	for (i=1; i<module->symTableSize/(sizeof(struct ElfSymbol)); i++)
	{
		symbol=&module->symTable[i];

		if (symbol->symIndex == STN_UNDEF)
			continue;

		if (ELF_ST_TYPE(symbol->symInfo) != type)
			continue;

		if (strcmp(symName, module->strTable+symbol->symName))
			continue;

		/* Handle data? */
		return (void*)(symbol->symValue);
	}

	return NULL;
}

#ifdef CONFIG_ALL_SYMBOLS

void ModuleSymbolPrint(DWORD address)
{
	struct Module* curr, *module=NULL;

	ListForEachEntry(curr, &moduleList, next)
	{
		if (address >= curr->textAddr && address < curr->textAddr+curr->textLength)
		{
			module=curr;
			break;
		}
	}

	if (module)
	{
		DWORD size, offset;
		const char* SymbolLookup(char*, struct ElfSymbol*, int, DWORD, DWORD*, DWORD*);

		const char* name=SymbolLookup(module->strTable, module->symTable, module->symTableSize, address, &size, &offset);

		if (name)
			KePrint("%s+%#X/%#X (module)\n", name, offset, size);
		else
			KePrint("%#X\n", address);
	}
}

void ModuleSymbolAdd(struct Module* module)
{
	int length=module->symTableSize+module->strTableSize;
	char* dest=(void*)VirtMapPhysRange(MODULE_START, MODULE_END, PAGE_ALIGN_UP(length) >> PAGE_SHIFT, 3);

	memcpy(dest, module->symTable, module->symTableSize);
	module->symTable=(struct ElfSymbol*)dest;

	dest+=module->symTableSize;
	memcpy(dest, module->strTable, module->strTableSize);
	module->strTable=dest;	
}

#else
void ModuleSymbolPrint(DWORD address)
{
	KePrint("%#X\n", address);
}

void ModuleSymbolAdd(struct Module* module)
{
}
#endif

SYMBOL_EXPORT(ModuleSymbolPrint);

void ModulePrintModules()
{
	struct Module* curr=NULL;
	
	ListForEachEntry(curr, &moduleList, next)
		KePrint("MODULE: %s: %#X to %#X\n", curr->name, curr->textAddr, curr->textAddr+curr->textLength);
}

DWORD ModuleResolveKernel(char* name)
{
	/* Look through the kernel symbol table. */
	struct KernelSymbol* currSym=(struct KernelSymbol*)symtable_start;

	while (currSym != ((struct KernelSymbol*)symtable_end))
	{
		if (!strcmp(currSym->name, name))
			return currSym->addr;

		currSym++;
	}

	struct Module* curr;

	PreemptDisable();

	/* Look through the kernel symbol table of different modules. */
	ListForEachEntry(curr, &moduleList, next)
	{
		if (!curr->keSymTab)
			continue;

		currSym=(struct KernelSymbol*)(curr->keSymTab);
		int i=0;

		while (i < curr->keSymTabSize)
		{				
			if (!strcmp(currSym->name, name))
			{
				PreemptEnable();
				return currSym->addr;
			}

			currSym++;
			i+=sizeof(struct KernelSymbol);
		}
	}

	PreemptEnable();

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: ModuleSymbolPrepare
 *
 * DESCRIPTION: Iterate through the symbol table and relocate them to the
 *				base address by altering their symValue, ready for
 *				ModuleSymbolResolve to update the relocations.
 *
 * PARAMETERS: module - the module's whose symbols are to be relocated.
 *
 * RETURNS: 0 on success, -ENOENT on an invalid symbol.
 *
 ***********************************************************************/

int ModuleSymbolPrepare(struct Module* module)
{
	DWORD i;
	DWORD symSize=module->symTableSize/sizeof(struct ElfSymbol);

	/* Iterate through all the symbols in the symbol table, skipping the first entry,
	 * which is a NULL symbol. */
	for (i=1; i<symSize; i++)
	{
		struct ElfSymbol* symbol=&module->symTable[i];

		/* Get the symbol's name in the string table. */
		char* symName=module->strTable+symbol->symName;

		switch (symbol->symIndex)
		{
			/* The symbol is undefined, so we've got to look at the kernel
			 * and module symbols to set its value. */
			case STN_UNDEF:
				symbol->symValue = ModuleResolveKernel(symName);

				if (symbol->symValue)
					break;

				KePrint(KERN_ERROR "Could not resolve %s\n", symName);

				return -ENOENT;

			/* Absolute symbol. No relocation needed. */
			case STN_ABS:
				break;

			/* Common symbols aren't supported by the module loading code. */
			case STN_COMMON:
				KePrint("Please recompile the module with -fno-common.\n");
				return -ENOENT;

			/* Just add the section base to the symbol value. */
			default:
				symbol->symValue+=module->sectionHeaders[symbol->symIndex].shAddr;
		}
	}

	return 0;
}

void ModuleSymbolResolve(struct Module* module, char* file, int sec)
{
	DWORD i;
	struct ElfReloc* reloc;
	struct ElfReloc* relTable=(struct ElfReloc*)(file+module->sectionHeaders[sec].shOffset);
	DWORD relSize=module->sectionHeaders[sec].shSize/sizeof(struct ElfReloc);

	if (!module->sectionHeaders[ module->sectionHeaders[sec].shInfo ].shAddr)
		return;

	for (i=0; i<relSize; i++)
	{
		reloc=&relTable[i];
		unsigned long* relAddr=(unsigned long*)(module->sectionHeaders[ module->sectionHeaders[sec].shInfo ].shAddr+reloc->addr);
		int symTabIdx=ELF_R_SYM(reloc->info);
		struct ElfSymbol* symbol=&module->symTable[symTabIdx];

		switch (ELF_R_TYPE(reloc->info))
		{
			case R_386_NONE:
				break;

			case R_386_32:
				*relAddr+=symbol->symValue;
				break;

			case R_386_PC32:
				*relAddr+=symbol->symValue-(DWORD)relAddr;
				break;

			default:
				KePrint("ModuleSymbolResolve: type = %u, %u\n", ELF_R_TYPE(reloc->info), symbol->symIndex);
		}
	}
}

void* ModuleSectionFind(struct Module* module, const char* sectionNames, const char* name, int total, int* size)
{
	int i;

	for (i=0; i<total; i++)
	{
		/* If it's not allocated, it won't be in the final module image. */
		if (!(module->sectionHeaders[i].shFlags & SEC_FLAGS_ALLOC))
			continue;

		if (!strcmp(sectionNames+module->sectionHeaders[i].shName, name))
		{
			if (size)
				*size=module->sectionHeaders[i].shSize;

			return (void*)(module->sectionHeaders[i].shAddr);
		}
	}

	return NULL;
}

int ModuleAdd(char* name, void* file, void* image, unsigned long length, int flags)
{
	struct ElfHeader* elfHeader=(struct ElfHeader*)file;
	struct Module* module;
	struct ElfSectionHeader* sectionHeaders;
	DWORD loadAddr=(DWORD)image;
	int i, ret=0;

	if (!ELF_HEAD_CHECK(elfHeader) || (elfHeader->fileType != ELF_REL) || (elfHeader->shEntrySize != sizeof(struct ElfSectionHeader)))
		return -EINVAL;

	/* Iterate through section headers. */
	sectionHeaders=(struct ElfSectionHeader*)((char*)file+elfHeader->shOffset);

	/* Get basic information about the module. */
	module=(struct Module*)MemAlloc(sizeof(struct Module));

	module->name = name;
	module->loadAddr=loadAddr;
	module->sectionHeaders=sectionHeaders;

	if (!image)
		return -EFAULT;

	ZeroMemory(image, PAGE_ALIGN_UP(length));

	for (i=0; i<elfHeader->shEntries; i++)
	{
		/* Save the symbol table off for resolving later. */
		if (sectionHeaders[i].shType == SEC_TYPE_SYMTAB)
		{
			module->symTable=(struct ElfSymbol*)(file+sectionHeaders[i].shOffset);
			module->symTableSize=sectionHeaders[i].shSize;
		}

		if (sectionHeaders[i].shType == SEC_TYPE_STRTAB)
		{
			module->strTable=(char*)file+sectionHeaders[i].shOffset;
			module->strTableSize=sectionHeaders[i].shSize;
		}

		/* Align up sizes. */
		sectionHeaders[i].shSize=SIZE_ALIGN_UP(sectionHeaders[i].shSize, sectionHeaders[i].shAddrAlign);
	}

	/* Copy over the module data into a new image. */
	char* dest;
	dest=(char*)module->loadAddr;

	for (i=0; i<elfHeader->shEntries; i++)
	{
		if (!(sectionHeaders[i].shFlags & SEC_FLAGS_ALLOC))
			continue;

		if (sectionHeaders[i].shFlags & SEC_FLAGS_EXEC)
		{
			module->textAddr=(DWORD)dest;
			module->textLength=sectionHeaders[i].shSize;
		}

		sectionHeaders[i].shAddr=(DWORD)dest;

		/* Unless it's a BSS section (or similar), copy the data over. */
		if (sectionHeaders[i].shType != SEC_TYPE_NOBITS)
			memcpy(dest, (char*)(file+sectionHeaders[i].shOffset), sectionHeaders[i].shSize);

		dest+=sectionHeaders[i].shSize;
	}

	if (ModuleSymbolPrepare(module))
		return -ENOENT;

	for (i=0; i<elfHeader->shEntries; i++)
	{
		if (sectionHeaders[i].shType == SEC_TYPE_REL)
			/* Resolve entries in the module. */
			ModuleSymbolResolve(module, file, i);
		else if (sectionHeaders[i].shType == SEC_TYPE_RELA)
			KePrint("RELA\n");
	}

	/* Get the symbol table and string table, so we can resolve other symbols later. */
	module->keSymTab=ModuleSectionFind(module, 
		(char*)file+sectionHeaders[elfHeader->strTabSectionIndex].shOffset, 
			".symtable", elfHeader->shEntries, &module->keSymTabSize);

	/* Find the init and exit functions, using the symbol and string table addresses. */
	int (*modInit)(void);

	modInit = ModuleSymbolFind(module, "_ModuleInit", STT_FUNC);

	/* Support for Linux .ko modules. The module startup function can be found
	 * at init_module. */
	 
	if (!modInit)
		modInit = ModuleSymbolFind(module, "init_module", STT_FUNC);

	PreemptDisable();
	ListAddTail(&module->next, &moduleList);
	PreemptEnable();

	if (modInit)
		/* Call ModuleInit */
		ret=(*modInit)();
	
	KeObjectCreate(&module->object, &moduleSet, module->name);

	return ret;
}

int SysModuleAdd(char* name, void* data, unsigned long length);
int SysModuleRemove(const char* name);

struct SysCall moduleSysCalls[]={
	SysEntry(SysModuleAdd, 12),
	SysEntry(SysModuleRemove, 4),
	SysEntryEnd()
};

void ModulesBootLoad()
{
	ArchModulesLoad();

	/* Add the calls to the system table. */
	SysRegisterRange(SYS_MODULE_BASE, moduleSysCalls);
}

int SysModuleAdd(char* name, void* data, unsigned long length)
{
	void* kData;
	int ret;
	
	/* Check data is ok to access. */
	kData=(void*)VirtMapPhysRange(MODULE_START, MODULE_END, PAGE_ALIGN_UP(length) >> PAGE_SHIFT, 3);

	ret=ModuleAdd(name, data, kData, length, 0);

	return ret;
}

int SysModuleRemove(const char* name)
{
	KePrint("SysModuleRemove\n");
	return 0;
}

int ModuleRequest(const char* fmt, ...)
{
	VaList list;
	char* name;
	
	VaStart(list, fmt);
	name = vasprintf(fmt, list);
	VaEnd(list);
	
	/* TODO: Call userspace service that will load modules based on name, device
	 * ID or lack of system call
	 */
	
	MemFree(name);
	
	return 0;
}

void ModuleRelease(struct KeObject* object)
{
	KePrint("ModuleRelease\n");
}

int ModuleInfoInit()
{
	return KeSetCreate(&moduleSet, NULL, &moduleType, "Modules");
}
