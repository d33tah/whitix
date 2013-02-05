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

#include <print.h>
#include <typedefs.h>
#include <error.h>
#include <fs/vfs.h>
#include <sys.h>

extern int ArchReboot();

/***********************************************************************
 *
 * FUNCTION: SysShutdown
 *
 * DESCRIPTION: Shutdown or reboot the computer.
 *
 * PARAMETERS: None.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

int SysShutdown(int type)
{
	KePrint("Synchronising the filesystem\n");
//	SysFileSystemSync();
	KePrint("Rebooting\n");
	ArchReboot();

	/* Should not get here. */
	return 0;
}

struct SysCall sysShutdown=SysEntry(SysShutdown, 4);

void ShutdownInit()
{
	SysRegisterCall(SYS_SHUTDOWN_BASE, &sysShutdown);
}
