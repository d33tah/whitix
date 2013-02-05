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

#ifndef IOPORTS_H
#define IOPORTS_H

#include <typedefs.h>

/* All the functions for accessing the i386 I/O port space */

static inline BYTE inb(WORD port) 
{
	BYTE retVal;
	asm volatile("inb %w1,%b0" : "=a"(retVal) : "d"(port));
	return retVal;
}

#define outb(port,data) asm volatile("outb %%al,%%dx"::"d"(port),"a"(data))

static inline WORD inw(WORD port)
{
	WORD retVal;
	asm volatile("inw %%dx,%%ax" : "=a"(retVal) : "d"(port));
	return retVal;
}

#define outw(port,data)	asm volatile("outw %%ax,%%dx"::"d"(port),"a"(data))

static inline DWORD ind(WORD port)
{
	DWORD retVal;
	asm volatile("inl %%dx,%%eax" : "=a"(retVal) : "d"(port));
	return retVal;
}

#define outd(port,data)	asm volatile("outl %%eax,%%dx"::"d"(port),"a"(data))

/* String versions. */

static inline void insw(unsigned short port, void* addr, unsigned long count)
{
	asm volatile("cld; rep; insw" : "=D"(addr), "=c"(count) : "d"(port), "0"(addr), "1"(count));
}

static inline void insd(unsigned short port, void* addr, unsigned long count)
{
	asm volatile("cld; rep; insl" : "=D"(addr), "=c"(count) : "d"(port), "0"(addr), "1"(count));
}

#endif
