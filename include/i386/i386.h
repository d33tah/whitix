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

#ifndef I386_H
#define I386_H

#include <typedefs.h>

/* General i386-isms */

#define PAGE_SIZE 4096
#define PAGE_MASK (~0x00000FFF)
#define PAGE_SHIFT 12
#define PAGE_ALIGN_UP(address) (((address)+PAGE_SIZE-1) & PAGE_MASK)
#define PAGE_ALIGN(address) ((address) & PAGE_MASK)
#define PAGE_OFFSET(address) ((address) & (PAGE_SIZE-1))

/* cr0 flags - move perhaps? */
#define CR0_PG 0x80000000 /* Paging enable*/
#define CR0_CD 0x40000000 /* Cache disable */
#define CR0_NW 0x20000000 /* No write-through */

/* CR4 flags */
#define CR4_VME		0x00000001 /* Virtual 8086 mode extension. */
#define CR4_PVI		0x00000002 /* Protection virtual interrupt. */
#define CR4_TSD		0x00000004 /* Time-stamp disable. */
#define CR4_DE		0x00000008
#define CR4_PSE		0x00000010
#define CR4_PAE		0x00000020
#define CR4_MCE		0x00000040
#define CR4_PGE		0x00000080
#define CR4_PCE		0x00000100
#define CR4_OSFXSR	0x00000200
#define CR4_XMME	0x00000400

#define ARCH_STACK_TOP 0xC0000000
#define ARCH_STACK_SIZE	4096 /* Kernel stack size */

/* GDT */
#define GDT_TLS_OFFSET	48
#define GDT_KCODE_SELECTOR	0x08
#define GDT_TLS_SELECTOR	0x33

/* Used for getting register data in an interrupt */

struct Context
{
	DWORD esp0,ebp,edi,esi,edx,ecx,ebx,eax,gs,fs,es,ds,irq,error,eip,cs,eflags,esp3,ss3;
}PACKED;

struct CallContext
{
	DWORD eax, esp, ebp, edi, esi, edx, ecx, ebx, eflags;
}PACKED;

#define SetCr4(features) asm volatile("mov %0, %%cr4" :: "r"((features | GetCr4())))

static inline DWORD GetCr4()
{
	DWORD retVal;
	asm volatile("mov %%cr4, %0" : "=r"(retVal));
	return retVal;
}

/* Utility functions for paging */

#define SetCr3(pageDir) asm volatile("mov %0,%%cr3" ::"r"((pageDir)))

static inline DWORD GetCr3()
{
	DWORD retVal;
	asm volatile("mov %%cr3,%0" : "=r"(retVal));
	return retVal;
}

static inline DWORD GetCr0()
{
	DWORD retVal;
	asm volatile("mov %%cr0,%0" : "=r"(retVal));
	return retVal;
}

/* Intel recommends a jump after enabling paging. */

static inline void SetCr0(DWORD cr0)
{
	asm volatile("mov %0,%%cr0\n\t"
				 "jmp 1f\n\t"
				 "1:" :: "r"(cr0));
}

void IrqInit();

/* Move and document! */

struct FSave
{
	long cwd;
	long swd;
	long twd;
	long fip;
	long fcs;
	long foo;
	long fos;
	long stSpace[20];
	long status;
}PACKED;

struct FxSave
{
	unsigned short cwd;
	unsigned short swd;
	unsigned short twd;
	unsigned short fop;
	long fip;
	long fcs;
	long foo;
	long fos;
	long mxcsr;
	long mxcsr_mask;
	long st_space[32];
	long xmm_space[32];
	long padding[56];
}__attribute__((aligned (16)));

union FpuSave
{
	struct FSave fSave;
	struct FxSave fxSave;
};

/* Used to halt the machine */
#define MachineHalt() do { cli(); hlt(); } while(0)

void ArchSwitchStackCall(void* stackP,void* function,void* arg);

/* General functions. */
static inline unsigned long _xchg(unsigned long x, volatile void* ptr, int size)
{
	switch (size)
	{
		case 1:
			asm volatile("xchgb %b0, %1" : "=q"(x) : "m"(ptr), "0"(x) : "memory");
			break;

		case 2:
			asm volatile("xchgw %w0, %1" : "=r"(x) : "m"(ptr), "0"(x) : "memory");
			break;

		case 4:
			asm volatile("xchgl %0, %1" : "=r"(x) : "m"(ptr), "0"(x) : "memory");
			break;
	}

	return x;
}

#define xchg(ptr, x) ((typeof(*ptr))_xchg((unsigned long)x, (ptr), sizeof(*(ptr))))

#endif
