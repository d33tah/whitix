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
#include <sections.h>

/* Definition of IDT entry trampolines */

extern void int0();
extern void int1();
extern void int2();
extern void int3();
extern void int4();
extern void int5();
extern void int6();
extern void int7();
extern void int8();
extern void int9();
extern void intA();
extern void intB();
extern void intC();
extern void intD();
extern void intE();
extern void intF();
extern void int10();
extern void int11();
extern void int12();
extern void int13();

extern void hwIrq32();
extern void hwIrq33();
extern void hwIrq34();
extern void hwIrq35();
extern void hwIrq36();
extern void hwIrq37();
extern void hwIrq38();
extern void hwIrq39();
extern void hwIrq40();
extern void hwIrq41();
extern void hwIrq42();
extern void hwIrq43();
extern void hwIrq44();
extern void hwIrq45();
extern void hwIrq46();
extern void hwIrq47();
extern void intSysEntry();

#define IDT_SIZE 100 /* Shouldn't need more than 100, and saves memory */
#define HwIdtLoad(idt) asm volatile("lidt %0" :: "m"(idt))

typedef void (*IdtStub)();

struct IdtEntry idtEntries[IDT_SIZE];
struct Idt idt;

#define IDT_INTERRUPT 14 /* Interrupts automatically disabled on entry */
#define IDT_TRAP 15 /* Interrupt flag left intact on entry */

/***********************************************************************
 *
 * FUNCTION: IdtSetEntry
 *
 * DESCRIPTION: Add a IDT entry to the table in memory.
 *
 * PARAMETERS: index - index into the IDT.
 *			   stubAddress - address of the code trampoline.
 *			   privLevel - privilege level, pl0 to pl3.
 *			   type - type of entry - interrupt or trap.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

initcode static void IdtSetEntry(int index,IdtStub stubAddress,int privLevel,int type)
{	
	idtEntries[index].lowOffset=(DWORD)stubAddress & 0xFFFF;
	idtEntries[index].selector=0x08; /* Kernel code selector */
	idtEntries[index].settings=(privLevel << 13) | 0x8000 | (type << 8);
	idtEntries[index].highOffset=(DWORD)stubAddress >> 16;
}

/***********************************************************************
 *
 * FUNCTION: IdtSetInt
 *
 * DESCRIPTION: Sets an interrupt - interrupts disabled on entry and only
 *				accessible from pl0.
 *
 * PARAMETERS: index - index into the IDT
 *			   stubAddress - address of the code trampoline.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

initcode static void IdtSetInt(int index,IdtStub stubAddress)
{
	IdtSetEntry(index,stubAddress,0,IDT_INTERRUPT);
}

initcode static void IdtSetTrap(int index,IdtStub stubAddress)
{
	IdtSetEntry(index,stubAddress,0,IDT_TRAP);
}

initcode static void IdtSetSystemGate(int index,IdtStub stubAddress)
{
	IdtSetEntry(index,stubAddress,3,IDT_TRAP);
}

initcode static void IdtSetSystemInt(int index,IdtStub stubAddress)
{
	IdtSetEntry(index,stubAddress,3,IDT_INTERRUPT);
}

initcode void IdtInit()
{
	/* Set up hardware exceptions */
	IdtSetTrap(0,int0); /* Divide by zero */
	IdtSetInt(1,int1); 
	IdtSetInt(2,int2);
	IdtSetSystemInt(3,int3); /* Debug interrupt */
	IdtSetSystemGate(4,int4);
	IdtSetSystemGate(5,int5);
	IdtSetInt(6,int6);
	IdtSetInt(7,int7);
	IdtSetInt(8,int8); /* Double fault */
	IdtSetInt(9,int9);
	IdtSetInt(10,intA);
	IdtSetInt(11,intB);
	IdtSetInt(12,intC);
	IdtSetInt(13,intD); /* General protection fault */
	IdtSetInt(14,intE); /* Page fault */
	IdtSetInt(15,intF);
	IdtSetInt(16,int10);
	IdtSetInt(17,int11);

	IdtSetInt(19, int13); /* SIMD co-processor error. */
	
	/* Set all the hardware irq entries */
	IdtSetInt(32,hwIrq32);
	IdtSetInt(33,hwIrq33);
	IdtSetInt(34,hwIrq34);
	IdtSetInt(35,hwIrq35);
	IdtSetInt(36,hwIrq36);
	IdtSetInt(37,hwIrq37);
	IdtSetInt(38,hwIrq38);
	IdtSetInt(39,hwIrq39);
	IdtSetInt(40,hwIrq40);
	IdtSetInt(41,hwIrq41);
	IdtSetInt(42,hwIrq42);
	IdtSetInt(43,hwIrq43);
	IdtSetInt(44,hwIrq44);
	IdtSetInt(45,hwIrq45);
	IdtSetInt(46,hwIrq46);
	IdtSetInt(47,hwIrq47);

	IdtSetSystemGate(48, intSysEntry); /* Syscall */
}

/***********************************************************************
 *
 * FUNCTION: IdtLoad
 *
 * DESCRIPTION: Load the IDT into the processor's IDT register
 *
 * PARAMETERS: None.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

initcode void IdtLoad()
{
	idt.limit=IDT_SIZE*(sizeof(struct IdtEntry)-1);
	idt.entries=idtEntries;

	HwIdtLoad(idt);
}
