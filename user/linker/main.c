#include "syscalls.h"
#include "relocate.h"
#include "lib.h"

int dummy;

struct ElfResolve lResolve;

static unsigned long __attribute((used)) LinkerMain(int argc,char** argv)
{
	unsigned long* args=((unsigned long*)&argv[0]);
	struct ElfAuxEnt elfEntries[AT_EGID+1];
	struct ElfDyn* dynEntries;
	struct ElfResolve resolve;
	int i;

	args+=argc;
	args++; /* Skip over NULL */

	resolve.libName="Linker";

	/* Save off the entries */
	while (*args)
	{	
		struct ElfAuxEnt* currEnt=(struct ElfAuxEnt*)args;
		if (currEnt->id <= AT_EGID)
			link_memcpy((char*)&elfEntries[currEnt->id],(char*)currEnt,sizeof(struct ElfAuxEnt));
		args+=2;
	}

	resolve.loadAddr=elfEntries[AT_BASE].value;
	if (!resolve.loadAddr)
		resolve.loadAddr=GetElfLoadAddress();
	
	dynEntries=(struct ElfDyn*)(ElfGetDynamic() + resolve.loadAddr);

	ElfParseDynInfo(dynEntries, resolve.dynamicInfo, resolve.loadAddr);

	/* Relocate ourselves; can't even call other functions until we do this. (The above
	 * function calls are all to inline functions). */
	for (i=0; i<2; i++)
	{
		int relAddr=i ? resolve.dynamicInfo[DT_JMPREL] : resolve.dynamicInfo[DT_REL];
		int relSize=i ? resolve.dynamicInfo[DT_PLTRELSZ] : resolve.dynamicInfo[DT_RELSZ];
		int relCount=resolve.dynamicInfo[DT_RELCOUNT_IDX];

		if (!relAddr)
			continue;

		if (!i && relCount)
		{
			relSize-=relCount*sizeof(struct ElfReloc);
			ElfRelative(resolve.loadAddr, relAddr, relCount);
			relAddr+=relCount+sizeof(struct ElfReloc);
		}
	}

	/* Fill in the rest of the resolve structure. */
	resolve.pHeaders = (struct ElfProgHeader*)elfEntries[AT_PHDR].value;
	resolve.dynamicAddr = dynEntries;
	
	/* Copy it over to a global variable, as it will be added to a linked list */
	link_memcpy(&lResolve, &resolve, sizeof(struct ElfResolve));
	
	/* Set up the thread local storage for the starting thread. */
	TlsInit();
	
	/* And relocate the program. */
	DlRelocate(elfEntries, &lResolve, argv);

	/* Now transfer execution to that address */
	return elfEntries[AT_ENTRY].value;
}

/* Don't need return address (if any) on the stack, just want the arguments. This function never
returns anyway. */

asm(
	"	.text\n"
	"	.globl	_start\n"
	"	.type	_start,@function\n"

	"_start:\n"
	"	add $0x04,%esp\n" /* Add so LinkerMain gets argc and argv arguments */
	"	call LinkerMain\n"
	"	sub $0x04,%esp\n"
	"	movl %eax,%edi\n"
	"	call 1f\n"
	"1:	popl	%ebx\n"
	"	addl $_GLOBAL_OFFSET_TABLE_+[.-1b],%ebx\n"
	"	leal DlFini@GOTOFF(%ebx), %edx\n"
	"	jmp *%edi\n"
);
