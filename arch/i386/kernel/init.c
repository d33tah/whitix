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

#include <i386/i386.h>
#include <i386/pit.h>
#include <i386/smp.h>
#include <i386/cpuid.h>
#include <i386/virtual.h>
#include <i386/idt.h>
#include <i386/ioports.h>
#include <i386/pic.h>
#include <print.h>
#include <time.h>
#include <vmm.h>
#include <acpi/acpi.h>

int AcpiEarlyInit();
int i386InitMultiBoot();
int SlabInit();
int SysArchInit();
int TlsInit();
int DelayInit();
int SibInit();

#define rdtsc(low,high) \
	asm volatile("rdtsc" : "=a"(low), "=d"(high))

#define CALIBRATE_LATCH (5*LATCH)
#define CALIBRATE_TIME  (5*1000020/HZ)

static DWORD CalibrateTsc()
{
	DWORD startLow,startHigh,endLow,endHigh,count;

	/* Set the gate high and disable the speaker */
	outb(0x61,(inb(0x61) & ~0x02) | 0x01);
	/* Use the PIT to countdown */
	outb(0x43,0xB0);
	outb(0x42,CALIBRATE_LATCH & 0xFF);
	outb(0x42,CALIBRATE_LATCH >> 8);

	rdtsc(startLow,startHigh);
	count=0;
	do
	{
		++count;
	}while (!(inb(0x61) & 0x20));

	rdtsc(endLow,endHigh);

	/* Do a 64-bit subtract */
	asm("subl %2,%0 \n\t"
		"sbbl %3,%1" : "=a"(endLow),"=d"(endHigh)
		: "g"(startLow),"g"(startHigh),"g"(endLow),"1"(endHigh));

	asm("divl %2" : "=a"(endLow),"=d"(endHigh) : "r"(endLow),"g"(0),"1"(CALIBRATE_TIME));
	return endLow;
}

/* Could probably be moved into pit.c */
static void CalcCpuSpeed()
{
	/* Adapted from Linux, which seems to have a pretty accurate CPU speed calculator. */
	if (CpuIdSupports(CPU_FEATURE_TSC))
	{
		DWORD tscQ=CalibrateTsc();
		DWORD eax=0,edx=1000,cpuKhz;
		asm("divl %2" : "=a"(cpuKhz),"=d"(edx):"r"(tscQ),"g"(eax),"1"(edx));

		KePrint("CPU: Detected a %lu.%03lu MHz processor.\n",cpuKhz/1000,cpuKhz % 1000);
	}
}

int VirtEarlyInit();
int IoPermsInit();

int ArchInit()
{
	KePrint(KERN_INFO "Whitix x86 v0.2b, built on %s at %s\n",__DATE__,__TIME__);
	i386InitMultiBoot();
	PhysInit();
	VirtEarlyInit();
	CpuIdInit();
	CalcCpuSpeed();

	SlabInit();
	
	SmpInit();
	
	VirtInit();	
	MMapInit();
	
//	AcpiInit();
	
	PicRemap(0x20,0x28);
	IrqInit();
	IdtInit();
	IdtLoad();

	SysArchInit();
	SibInit();

	IoPermsInit();
	TlsInit();

	PitInit();

	DelayInit();

	return 0;
}
