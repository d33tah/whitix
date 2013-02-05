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
#include <module.h>
#include <sections.h>
#include <symbols.h>
#include <print.h>

#ifdef CONFIG_ALL_SYMBOLS

#define KERNEL_BASE_ADDRESS		0x100000
#define DATA __attribute__((section(".data")))

static struct ElfSymbol* symTable DATA;
static int symTableSize DATA;
static char* strTable DATA;
static int strTableSize DATA;

const char* SymbolLookup(char* stringTable, struct ElfSymbol* symbolTable, int symSize, DWORD address, DWORD* size, DWORD* offset)
{
	DWORD i=0;
	struct ElfSymbol* currSym=NULL;
	struct ElfSymbol* symbol=NULL;
	DWORD currOffset=~0;

	while (i < (symSize/sizeof(struct ElfSymbol)))
	{
		symbol=&symbolTable[i++];

		if (ELF_ST_TYPE(symbol->symInfo) != STT_FUNC)
			continue;

		if (address-symbol->symValue > currOffset)
			continue;

		currOffset=address-symbol->symValue;

		currSym=symbol;
	}

	if (!currSym)
		return NULL;

	*offset=currOffset;
	*size=currSym->symSize;

	if (*offset > *size)
	{
		KePrint("%#X-%#X = %#X %#X\n", address, symbol->symValue, *offset, *size);
		return NULL;
	}

	return stringTable+currSym->symName;
}

void SymbolPrint(DWORD address)
{
	DWORD size, offset;
	const char* name;

	name = SymbolLookup(strTable, symTable, symTableSize, address, &size, &offset);

 	if (name)
		KePrint("%s+%#X/%#X\n", name, offset, size);
	else
		KePrint("%#X\n", address);
}

SYMBOL_EXPORT(SymbolPrint);

void SymbolsCopy()
{
	DWORD sectionAddr, numEntries;
	struct ElfSectionHeader* sectionHeaders;
	int i;
	
	EarlyConsoleInit();
	ArchGetSectionInfo(&sectionAddr, &numEntries);
	
	sectionHeaders=(struct ElfSectionHeader*)sectionAddr;

	/* Search the section headers for the symbol and string tables. */
	for (i=1; i<numEntries; i++)
	{
		struct ElfSectionHeader* curr=&sectionHeaders[i];
		
		if (curr->shType == SEC_TYPE_SYMTAB)
		{
			symTable=(struct ElfSymbol*)(curr->shAddr);
			symTableSize=curr->shSize;
		}else if (curr->shType == SEC_TYPE_STRTAB)
		{
			strTable=(char*)(curr->shAddr);
			strTableSize=curr->shSize;
		}
	}
}

#else
/* Stub functions. */

void SymbolPrint(DWORD address)
{
	KePrint("%10X\n", address);
}

void SymbolsCopy()
{
}

#endif
