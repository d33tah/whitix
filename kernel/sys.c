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

#include <sys.h>
#include <console.h>
#include <module.h>
#include <i386/virtual.h>
#include <types.h>

#include <print.h>

struct SysCall sysCallTable[SYSCALL_MAX];

void SysRegisterCall(int index, struct SysCall* call)
{
	if (index > SYSCALL_MAX)
		return;

	memcpy(&sysCallTable[index], call, sizeof(struct SysCall));
}

SYMBOL_EXPORT(SysRegisterCall);

void SysRegisterRange(int index, struct SysCall* calls)
{
	struct SysCall* currCall=calls;

	while (currCall->addr)
		SysRegisterCall(index++, currCall++);
}

SYMBOL_EXPORT(SysRegisterRange);
