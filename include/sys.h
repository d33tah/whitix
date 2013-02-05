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

#ifndef SYS_H
#define SYS_H

#include <typedefs.h>
#include <types.h>

struct SysCall
{
	DWORD addr;
	DWORD numBytes;
};

/* System call offsets. */

#define SYS_FS_BASE				0
#define SYS_MMAP_BASE			25
#define SYS_SHMEM_BASE			29
#define SYS_THREAD_BASE			33
#define SYS_LOAD_BASE			38
#define SYS_PROCESS_BASE		39

#define SYS_SCHED_BASE			42
#define SYS_TIME_BASE			43
#define NET_SYS_BASE			44
#define SYS_MODULE_BASE			53
#define SYS_CONFIG_BASE			55
#define SYS_SHUTDOWN_BASE		57
#define SYS_IO_BASE				58

#define SYS_TLS_BASE			59
#define SYSCALL_MAX		60

#define SysEntry(addr, bytes) { (DWORD)addr, bytes }
#define SysEntryEnd() SysEntry(0, 0)

void SysRegisterCall(int index, struct SysCall* call);
void SysRegisterRange(int index, struct SysCall* calls);

extern struct SysCall sysCallTable[SYSCALL_MAX];

__attribute__((fastcall))
static inline addr_t SysGetCall(DWORD index, DWORD argBytes)
{
	if (index > SYSCALL_MAX)
		return 0;

	/* The number of argument bytes past can be greater than the number of bytes
	 * needed by the function's parameters; it allows for future compatability.
	 */
	 
	if (sysCallTable[index].numBytes < argBytes)
		return 0;

	return sysCallTable[index].addr;
}

#endif
