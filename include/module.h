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

#ifndef MODULE_H
#define MODULE_H

#define ModuleInit(func) \
	int _ModuleInit(void) __attribute__((alias(#func)));

#define MODULE_START	0xD8000000
#define MODULE_END		0xE0000000

#ifndef MODULE /* Kernel-specific structures. */

#include <elf.h>
#include <llist.h>
#include <typedefs.h>
#include <keobject.h>

struct KernelSymbol;

struct Module
{
	char* name;
	struct ListHead next;

	struct KernelSymbol* keSymTab;
	int keSymTabSize;

	/* ELF specific information. */
	DWORD loadAddr;
	DWORD textAddr;
	DWORD textLength;

	struct ElfSectionHeader* sectionHeaders;

	char* strTable;
	int strTableSize;

	struct ElfSymbol* symTable;
	int symTableSize;
	
	struct KeObject object;
};

void ModuleSymbolPrint(DWORD address);
int ModuleAdd(char* name, void* file, void* image, unsigned long length, int flags);

#endif

struct KernelSymbol
{
	unsigned long addr;
	const char* name;
};

/* So drivers and stacks can access kernel functions, either through a trampoline
 * or directly. */

#define SYMBOL_EXPORT(sym) \
	static const char _KeStrTab_##sym[] \
	__attribute__((section(".symstrings"))) __attribute__((__used__)) \
	= #sym; \
	static const struct KernelSymbol _KeSymTab_##sym \
	__attribute__((section(".symtable"))) __attribute__((__used__)) \
	= { (unsigned long)&sym, _KeStrTab_##sym }

#endif
