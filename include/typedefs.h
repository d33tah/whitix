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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#define MIN(a,b) (((a < b) ? a : b))
#define MAX(a,b) (((a > b) ? a : b))

/* Some i386-isms */

#define hlt() asm volatile("hlt" ::: "memory")
#define cli() asm volatile("cli" ::: "memory")
#define sti() asm volatile("sti" ::: "memory")
#define nop() asm volatile("nop" ::: "memory")

/* EFLAGS functions. */
#define SaveFlags(flags) \
	asm volatile("pushfl ; popl %0" : "=g"(flags))

#define RestoreFlags(flags) \
	asm volatile("pushl %0 ; popfl" :: "g"(flags) : "memory","cc")

#define IrqSaveFlags(flags) do{ SaveFlags(flags); cli(); } while(0)
#define IrqRestoreFlags(flags) do{ RestoreFlags(flags); } while(0)

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef NULL
#define NULL 0
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;

/* GCC functions */
#define PACKED __attribute__((packed))
#define LIKELY(condition) __builtin_expect((condition),1)
#define UNLIKELY(condition) __builtin_expect((condition),0)

#define count(array) (sizeof(array)/sizeof(array[0]))

/* Useful */
#define PTR_ERROR(ptr) ((!(ptr)) ? -EFAULT : 0)

/* Check if IRQs are enabled, by looking at the EFLAGS register. */
static inline int IrqsEnabled()
{
	DWORD flags;
	SaveFlags(flags);
	return (flags >> 9) & 1;
}

/* Place somewhere else */
#define DEVICES_PATH "/System/Devices/"

#define USED(x) ((void)x)

#define RESULT_CHECK __attribute__((warn_unused_result))

/* General macros */
#define OffsetOf(type, member) __builtin_offsetof(type, member)

#define ContainerOf(ptr, type, member) ({ \
	const typeof( ((type *)NULL)->member ) * memPtr = (ptr);	\
	(type *)( (char *)memPtr - OffsetOf(type, member) );})

#define SIZE_ALIGN_UP(size, align) (((size)+(align-1)) & (~(align-1)))

#endif
