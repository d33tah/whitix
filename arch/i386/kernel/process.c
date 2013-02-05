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

#include <i386/process.h>
#include <i386/i386.h>
#include <task.h>
#include <typedefs.h>
#include <malloc.h>
#include <module.h>
#include <print.h>
#include <vmm.h>
#include <symbols.h>
#include <sections.h>

#include <i386/ioports.h>

void i386ExitThread();
void endi386ExitThread();
DWORD exitCode=0xF8000000;

struct TSS* tss=(struct TSS*)TSS_ADDR;

struct Thread* ThrArchSwitch(struct Thread* prev, struct Thread* next)
{
	DWORD flags;

	if (lastMathThread != next)
		SetCr0(GetCr0() | 8);

	IrqSaveFlags(flags);

	/* Copy over the new TLS for the thread. See boot/multiboot.S for the
	 * definition of the GDT. */
	TlsInstall(next);
	
	tss->esp0=(next->currStack & ~(ARCH_STACK_SIZE-1))+ARCH_STACK_SIZE;
	i386ContextSwitch(prev,next);

	IrqRestoreFlags(flags);

	return prev;
}

int ThrArchInit()
{
	/* Setup up the TSS, only needed for ss0 and esp0 */
	tss->bitmap=0xFFFF; /* Bitmap offset. Past limit, so every IO 
						instruction in userspace will produce a GPF. */

	tss->ss0=0x10; /* Kernel data descriptor */
	tss->esp0=0; /* An invalid stack pointer - will flag up if used. */

	return 0;
}

void ThrArchEnterUser()
{
	extern void i386EnterUserSpace(DWORD entry,DWORD esp3);
	i386EnterUserSpace(currThread->entry,currThread->esp3);
}

/* 512k stack. */
#define I386_STACK_LENGTH 512*1024

void ThrArchSetupUserStack(struct Thread* ret, DWORD* stackP, void* argument)
{
	DWORD flags;
	DWORD* currP;

	if (!*stackP)
		/* Allocate own user stack. Used for when it doesn't really matter. */		*stackP=MMapDo(ret->parent, NULL, 0, I386_STACK_LENGTH, PAGE_RW | PAGE_PRESENT | PAGE_USER, 0, MMAP_PRIVATE, NULL)+I386_STACK_LENGTH;

	ret->esp3=*stackP; /* User stack */

	IrqSaveFlags(flags);
	VirtSetCurrent(ret->parent->memManager);

	currP=(DWORD*)(ret->esp3);

	if (argument != ((void*)-1))
		*--currP=(DWORD)argument;

	*--currP = 0;

	ret->esp3=(DWORD)currP;

	/* And restore the current process. */
	VirtSetCurrent(current->memManager);
	IrqRestoreFlags(flags);
}

int ThrArchCreateThread(struct Thread* ret,DWORD entry,int user,DWORD stackP, void* argument)
{
	DWORD* currP;
	int i;

	/* Allocate the kernel stack. */
	ret->currStack=(DWORD)MemAlloc(ARCH_STACK_SIZE);
	ret->currStack+=ARCH_STACK_SIZE;
	
	ret->entry=entry; /* eip */

	if (user)
		ThrArchSetupUserStack(ret, &stackP, argument);

	currP=(DWORD*)(ret->currStack);

	/* Push the given argument on the kernel stack if this is not a user thread. */
	if (!user)
		if (argument != ((void*)-1))
		{
			*--currP=(DWORD)argument;
			*--currP=0;
		}

	/* Is a kernel thread? */
	*--currP=(user) ? (DWORD)ThrArchEnterUser : entry;

	/* Zero all the general purpose registers - simulate a pusha. */
	for (i=0; i<8; i++)
		*--currP=0;

	ret->currStack=(DWORD)currP;

	return 0;
}

int ThrArchDestroyThread(struct Thread* thread)
{
	/* If one of the threads was the last math thread, make sure it no longer is.
	 * Otherwise the FPU exception will save to bogus memory.
	 */
	extern struct Thread* lastMathThread;

	if (thread == lastMathThread)
		lastMathThread = NULL;

	return 0;
}

#define PROCESS_STACK_LENGTH	100

void ThrArchPrintStack(DWORD* esp)
{
	int i;
	
	KePrint(KERN_ERROR "Stack trace:\n");
	for (i=0; i< PROCESS_STACK_LENGTH; i++,esp++)
	{
		if (*esp > (DWORD)code && *esp < (DWORD)endCode)
		{
			SymbolPrint(*esp);
		}else if (*esp > 0xD8000000 && *esp < 0xE0000000)
			ModuleSymbolPrint(*esp);
	}
}

void ThrArchPrintCurrStack()
{
	DWORD* currEsp;
	
	asm volatile("mov %%esp, %%eax" : "=a"(currEsp));
	
	ThrArchPrintStack(currEsp);
}

SYMBOL_EXPORT(ThrArchPrintCurrStack);

void ThrArchPrintContext(struct Context* curr)
{
	DWORD flags;
	DWORD* esp;

	IrqSaveFlags(flags);

	if (curr->cs & 0x3)
		esp=(DWORD*)curr->esp3;
	else
		esp=(DWORD*)curr->esp0;

	KePrint("Thread context:\n");
	KePrint("eax = %#08X ebx = %#08X ecx = %#08X edx = %#08X\n",curr->eax,curr->ebx,curr->ecx,curr->edx);
	KePrint("ebp = %#08X edi = %#08X esi = %#08X eip = %#08X\n",curr->ebp,curr->edi,curr->esi,curr->eip);
	KePrint("cs = %#02X ds = %#02X es = %#02X fs = %#02X gs = %#02X esp = %#08X\n",curr->cs,curr->ds,curr->es,curr->fs,curr->gs,curr->esp0);
	KePrint("esp3 = %#08X cr0 = %#X cr3 = %#X eflags = %#X\n",curr->esp3,GetCr0(),GetCr3(),curr->eflags);
	KePrint("irqs enabled = %u, currThread = %#X (%d)\n",(curr->eflags >> 9) & 1, currThread, currThread->id);

	if (current)
		KePrint("process id = %u process name = %s\n",current->pid,ThrGetProcName(current->pid));

	if (!(curr->cs & 0x3))
		ThrArchPrintStack(esp);

	IrqRestoreFlags(flags);
}

/* Define the idle thread, as the startup code must have the esp of it
   which is in the bss section */

DWORD idleStack[ARCH_STACK_SIZE];

struct Thread _idleTask={
	.currStack = (DWORD)idleStack+ARCH_STACK_SIZE,
};

struct Thread* idleTask=&_idleTask;
