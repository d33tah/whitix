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
#include <i386/cpuid.h>
#include <print.h>
#include <module.h>
#include <sections.h>

/* Saves the edx value of CPUID level 1. Used to check for RDTSC, SYSENTER and other instructions. 
 * Needs to be adjusted for multicore systems. */
DWORD features=0;
DWORD maxLevel=0;

DWORD CpuIdSupports(DWORD mask)
{
	return (features & mask);
}

/***********************************************************************
 *
 * FUNCTION: CpuId
 *
 * DESCRIPTION: Perform the actual CPUID call.
 *
 * PARAMETERS: level - level of CPUID (different levels contain different
 *					   information).
 *			   eax,ebx,ecx,edx - registers containing information.
 *
 ***********************************************************************/

void CpuId(DWORD level,DWORD* eax,DWORD* ebx,DWORD* ecx,DWORD* edx)
{
	asm volatile("cpuid" : 
		"=a"(*eax),
		"=b"(*ebx),
		"=c"(*ecx),
		"=d"(*edx)
		: "a"(level), "c"(0));
}

void CpuIdVendorId(char* str)
{
	DWORD eax;
	
	CpuId(0, &eax, (DWORD*)&str[0], (DWORD*)&str[8], (DWORD*)&str[4]);	
}

SYMBOL_EXPORT(CpuIdVendorId);

void CpuIdName(char* str)
{
	ZeroMemory(str, 48);
	
	if (maxLevel > 0x80000004)
	{
		CpuId(0x80000002, (DWORD*)&str[0], (DWORD*)&str[4], (DWORD*)&str[8], (DWORD*)&str[12]);
		CpuId(0x80000003, (DWORD*)&str[16], (DWORD*)&str[20], (DWORD*)&str[24], (DWORD*)&str[28]);
		CpuId(0x80000004, (DWORD*)&str[32], (DWORD*)&str[36], (DWORD*)&str[40], (DWORD*)&str[44]);
	}
}

SYMBOL_EXPORT(CpuIdName);

DWORD* CpuIdFeatures()
{
	return &features;
}

SYMBOL_EXPORT(CpuIdFeatures);

/***********************************************************************
 *
 * FUNCTION: CpuIdInit
 *
 * DESCRIPTION: Call CPUID to find out type of CPU and add information
 *				to a structure.
 *
 * PARAMETERS: None.
 *
 ***********************************************************************/

initcode int CpuIdInit()
{
	char str[12];
	DWORD eax,ebx,ecx; /* Used to ignore values. */

	KePrint(KERN_INFO "CPU: Identifying CPU...");
		
	CpuId(0,&maxLevel,(DWORD*)&str[0],(DWORD*)&str[8],(DWORD*)&str[4]);
	
	if (!strncmp(str,"GenuineIntel",11))
		KePrint("Intel CPU");
	else if (!strncmp(str,"AuthenticAMD",11))
		KePrint("AMD CPU");
	KePrint("\n");

	/* Get the features bitfield. */
	CpuId(1,&eax,&ebx,&ecx,&features);

	/* Enable SSE if supported by the processor. */
	if (CpuHasFxsr())
	{
		KePrint(KERN_INFO "CPU: Enabling fast FPU save and restore\n");
		SetCr4(CR4_OSFXSR);
	}

	if (CpuIdSupports(CPU_FEATURE_SSE))
	{
		KePrint(KERN_INFO "CPU: Enabling unmasked SIMD FPU exception\n");
		SetCr4(CR4_XMME);
	}
	
	return 0;
}
