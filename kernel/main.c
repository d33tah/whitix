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

#include <console.h>
#include <sched.h>
#include <string.h>
#include <task.h>
#include <typedefs.h>
#include <fs/vfs.h>
#include <time.h>
#include <print.h>
#include <sched.h>
#include <sections.h>

/* Init declarations. */
extern int EarlyConsoleInit();
extern int ArchInit();
extern int ThrEarlyInit();
extern int TimeInit();
extern int ShutdownInit();
extern int ModulesBootLoad();
extern int SlabInit();
extern int VmInit();
extern int IcInit();
extern int MMapInit();
extern int VfsInit();
extern int LoadInit();
extern int DevFsInit();
extern int DeviceInit();

void KernelMain()
{
	EarlyConsoleInit();
	ArchInit();
	ThrEarlyInit();
	TimeInit();
	ShutdownInit();
	
	/* Set up the device-related subsystems. */
	IcInit();

	/* Once we've set up the ICFS framework, there are several
	 * areas of Whitix (like the slab code) that need to expose
	 * their internal information.
	 */

	SlabInfoInit();
	
	ModuleInfoInit();
	
	ThrInit();
	
	VfsInit();
	DevFsInit();
	DeviceInit();
	
	MiscInit();
	
	LoadInit();

	StartInit();

	/* This is where the idle thread idles, after returning from StartInit in startup.c */
	ThrIdleFunc();
}

