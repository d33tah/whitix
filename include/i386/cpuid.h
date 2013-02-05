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

#ifndef CPUID_H
#define CPUID_H

#include <console.h>
#include <typedefs.h>

int CpuIdInit();
DWORD CpuIdSupports(DWORD mask);

/* Features. CPUID Level 1, edx */
#define CPU_FEATURE_FPU		0x00000001
#define CPU_FEATURE_VME		0x00000002
#define CPU_FEATURE_DE		0x00000004
#define CPU_FEATURE_PSE		0x00000008
#define CPU_FEATURE_TSC		0x00000010
#define CPU_FEATURE_MSR		0x00000020
#define CPU_FEATURE_PAE		0x00000040
#define CPU_FEATURE_MCE		0x00000080
#define CPU_FEATURE_CX8		0x00000100
#define CPU_FEATURE_APIC	0x00000200
#define CPU_FEATURE_SEP		0x00000800

#define CPU_FEATURE_FXSR	0x01000000
#define CPU_FEATURE_SSE		0x02000000
#define CPU_FEATURE_SSE2	0x04000000

#define CpuHasFxsr() (CpuIdSupports(CPU_FEATURE_FXSR))

/* Functions */
void CpuIdVendorId(char* str);

#endif
