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
 
/* General bit functions */

#ifndef BITMAP_H
#define BITMAP_H

#include <typedefs.h>

/* A good utility function really */
void BmapSetBit(void* array,DWORD index,int value);
/* Returns non-zero if set */
int BmapGetBit(void* array,DWORD index);

/* Adapted from Linux. */
static inline int BitFindFirstZero(void* array, int size)
{
	int d0, d1, d2; /* Temporary variables. */
	int result;
	
	if (!size)
		return 0;
		
	asm volatile(
	"movl $-1,%%eax\n\t"
 	"xorl %%edx,%%edx\n\t"
	"repe; scasl\n\t"
 	"je 1f\n\t"
 	"xorl -4(%%edi),%%eax\n\t"
 	"subl $4,%%edi\n\t"
 	"bsfl %%eax,%%edx\n"
 	"1:\tsubl %%ebx,%%edi\n\t"
 	"shll $3,%%edi\n\t"
 	"addl %%edi,%%edx"
 	:"=d" (result), "=&c" (d0), "=&D" (d1), "=&a" (d2)
	:"1" ((size + 31) >> 5), "2" (array), "b" (array) : "memory");

	return result;
}

#endif
