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

#include <i386/idt.h>
#include <i386/irq.h>
#include <i386/process.h>
#include <sched.h>
#include <task.h>
#include <malloc.h>
#include <module.h>
#include <sys.h>
#include <user_acc.h>
#include <vmm.h>
#include <preempt.h>
#include <print.h>
#include <i386/cpuid.h>

struct Thread* lastMathThread=NULL;
struct ListHead irqsHead[MAX_IRQS];

/* Add a interrupt service routine to the list of routines
 * for that particular irq.
 */

int IrqAddEntry(BYTE irq, struct IrqEntry* entry)
{
	if (irq >= MAX_IRQS)
		return -EINVAL;

	if (ListEmpty(&irqsHead[irq]))
		IrqEnable(irq);

	ListAdd(&entry->next,&irqsHead[irq]);

	return 0;
}

int IrqAdd(BYTE irq, Irq isr, void* data)
{
	struct IrqEntry* newEntry;

	newEntry=(struct IrqEntry*)MemAlloc(sizeof(struct IrqEntry));

	if (!newEntry)
		return -ENOMEM;

	newEntry->irq=isr;
	newEntry->data=data;

	return IrqAddEntry(irq, newEntry);
}

SYMBOL_EXPORT(IrqAdd);

int IrqRemove(BYTE irq,Irq isr,void* data)
{
	struct IrqEntry* entry;
	int found=0;

	if (irq >= MAX_IRQS)
		return -EINVAL;

	/* Search through all the isrs for 'isr' */
	ListForEachEntry(entry,&irqsHead[irq],next)
	{
		if (entry->irq == isr && entry->data == data)
		{
			found=1;
	 		break;
		}
	}

	if (!found)
		return -EINVAL;

	ListRemove(&entry->next);

	if (ListEmpty(&irqsHead[irq]))
		IrqDisable(irq);

	return 0;
}

SYMBOL_EXPORT(IrqRemove);

void IrqInit()
{
	/* Set up the irqsHead list */
	int i;

	for (i=0; i<16; i++)
		INIT_LIST_HEAD(&irqsHead[i]);
}

extern DWORD numSysCalls;
__attribute__((fastcall))
int DispatchSysCall(DWORD, DWORD, DWORD);

/* DoSysCall
 * ---------
 *
 * Verifies the syscall stack and index and calls DispatchSysCall in idtstubs.S */

__attribute__((fastcall))
int IrqSysCall(DWORD stackPointer, DWORD numArgBytes, DWORD index)
{
	DWORD funcAddr=SysGetCall(index, numArgBytes);

	if (!funcAddr)
		return -ENOSYS;

//	KePrint("IrqSysCall(%u)\n", index);

	/* Verify stack */
	return DispatchSysCall(numArgBytes, stackPointer, funcAddr);
}

/* Human-friendly name of all the processor exceptions */
char* exceptionName[]=
{
	"DIVIDE BY ZERO",
	"DEBUG EXCEPTION",
	"NMI",
	"BREAKPOINT",
	"OVERFLOW",
	"BOUND EXCEPTION",
	"INVALID INSTRUCTION",
	"FPU NOT AVAILABLE",
	"DOUBLE FAULT",
	"COPROCESSOR SEGMENT OVERRUN",
	"INVALID TSS",
	"SEGMENT NOT PRESENT",
	"STACK EXCEPTION",
	"GENERAL PROTECTION FAULT",
	"PAGE FAULT",
	"(RESERVED)",
	"FLOATING POINT ERROR",
	"ALIGNMENT CHECK",
	"MACHINE CHECK",
	"",
	"SIMD CO-PROCESSOR ERROR",
};

void IrqHandleTraps(struct Context* curr,DWORD irq)
{
	/* Set to 1 when a fault cannot be handled */
	int exception=0;
	DWORD address=0;

	switch (irq)
	{
	
	/* Temprorary */
	case 1:
		break;
	
	/* FPU NOT AVAILABLE.
	 *
	 * Assuming the computer we run on has an FPU, this exception is only
	 * fired when TS (task switch) is set in CR0, and that's only set if a previous
	 * task has used the FPU.
	 */
	 
	/* FIXME: Make sure SSE state saves and works. */

	case 7:

		asm volatile("clts"); /* Set again when scheduling */
		if (lastMathThread)
		{
			if (CpuHasFxsr())
				asm volatile("fxsave %0" :: "m"(lastMathThread->fpuData.fxSave));
			else
				asm volatile("fnsave %0" : "=m" (lastMathThread->fpuData.fSave));
		}else
			asm volatile("fnclex");

		lastMathThread=currThread;

		/* Now put in current thread's FPU data */
		if (currThread->flags & THR_USED_MATH)
		{
			if (CpuHasFxsr())
				asm volatile("fxrstor %0" :: "m"(lastMathThread->fpuData.fxSave));
			else
				asm volatile("frstor %0"::"m"(lastMathThread->fpuData));
		}else
		{
			asm volatile("fninit");
			currThread->flags|=THR_USED_MATH;
		}
		break;

	/* PAGE FAULT */
	case 14:
		asm volatile("mov %%cr2,%0" : "=r"(address));
		if (MmapHandleFault(current, address, curr->error))
			exception=1;
		break;

	default:
		exception=1;
	}

	/* The current process (or kernel) cannot continue */
	if (exception)
	{
		if (irq == 14 && (curr->cs & 0x03))
			VirtShowFault(address,curr->error);

		if (curr->cs & 0x03)
		{
			/* User process caused an exception */
			KePrint("Exception: %s (%#X), at %#X. Exiting process.\n",exceptionName[irq],address,curr->eip);
			ThrArchPrintContext(curr); /* Debug */
			ThrProcessExit(-1);
			thrNeedSchedule=true;
		}else{
			cli(); /* Just get the message out */
			if (irq == 14 && address < PAGE_SIZE)
				KePrint("\nNULL POINTER EXCEPTION (address = %#.8X)\n", address);
			else
				KePrint("KERNEL EXCEPTION\nProcessor exception: %s (%#X)\n",exceptionName[irq], address);
			ThrArchPrintContext(curr);
			hlt();
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:	IrqHandleHw
 *
 * DESCRIPTION: Call each IRQ handler to see if the IRQ was expected - and
 *				exit if it was handled OK.
 *
 * PARAMETERS:	irq - interrupt request number (from 0 - 16)
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

void IrqHandleHw(DWORD irq)
{
	struct IrqEntry* curr;

	ListForEachEntry(curr,&irqsHead[irq],next)
		if (curr->irq(curr->data) == IRQ_HANDLED)
			break;
			
	IrqAccept(irq);
}

/***********************************************************************
 *
 * FUNCTION: IrqHandle
 *
 * DESCRIPTION: Handles interrupts and system calls.
 *
 * PARAMETERS: curr - the current thread's context.
 *
 * RETURNS: Nothing - esp is changed in ThrArchSwitch
 *
 ***********************************************************************/

void IrqHandle(struct Context curr)
{
	DWORD irq=curr.irq;

	if (currThread)
		currThread->currContext=&curr;

	switch (irq)
	{
		case 0x00 ... 0x1F:
			IrqHandleTraps(&curr,irq);
			break;

		case 0x20 ... 0x2F:
			IrqHandleHw(irq-0x20);
			break;
			
		default:
			KePrint("Interrupt %#X unhandled.\n", irq);
	}

	if (thrNeedSchedule && PreemptCanSchedule())
		ThrSchedule();
}

