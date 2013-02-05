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
#include <console.h>
#include <task.h>
#include <i386/process.h>
#include <sys.h>

int SysIoAccess(int on)
{
	struct CallContext* context;
	
	context = (struct CallContext*)currThread->currContext;
	
	if (on)
		/* Change the IOPL flag in eflags to 3, just for this process. */
		context->eflags |= (1 << 13) | (1 << 12);
	else
		context->eflags &= ~((1 << 13) | (1 << 12));

	return 0;
}

static struct SysCall ioSysCall=SysEntry(SysIoAccess, 4);

void IoPermsInit()
{
	SysRegisterCall(SYS_IO_BASE, &ioSysCall);
}
