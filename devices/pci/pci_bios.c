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


#include <i386/ioports.h>
#include <llist.h>
#include <pci.h>
#include <slab.h>
#include <print.h>

#define BIOS32_SIGNATURE  (('_' << 0) + ('3' << 8) + ('2' << 16) + ('_' << 24))
BYTE bios32FarCall[6];

int PciDetectBios()
{
#if 0
	DWORD addr=0xE0000,name,temp;
	BYTE length,sum,revision,err;

	for (;addr < 0xFFFF0; addr+=16)
	{
		printf("Here, %#X\n", addr);
		name=*(DWORD*)addr;
		if (name != 0x5F32335F)
			continue;

		//Structure length
		length=(*(BYTE*)addr+9)*16;

		if (length == 0)
			continue;

		for (sum=0; length!=0; length--)
			sum+=*(BYTE*)((addr+length)-1);

		*(DWORD*)(bios32FarCall+0)=*(DWORD*)(addr+4);
		*(WORD*)(bios32FarCall+4)=0x08;

		asm volatile("mov $0x49435024,%%eax\n"
			     "mov 0,%%ebx\n"
			     "lcall *(%%edi)"
			     : "=a"(err),
				"=b"(addr),
				"=c"(length),
				"=d"(temp)
			     :  "D"(&bios32FarCall));

		if (err == 0x80)
		{
			printf("Computer has BIOS32, but no PCI32\n");
			return false;
		}

		if (err)
		{
			printf("BIOS32 error = %u\n",(DWORD)err);
			return false;
		}

		return true;
	}

	return false;
#endif

	return 0;
}
