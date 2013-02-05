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

#include <module.h>
#include <panic.h>
#include <i386/i386.h>
#include <i386/process.h>
#include <console.h>
#include <print.h>

/***********************************************************************
 *
 * FUNCTION: KernelPanic
 *
 * DESCRIPTION: Halts the machine. Called in the *worst* cases, when an
 *				unrecoverable event occurs. CPU faults are dealt with
 *				differently.
 *
 * PARAMETERS: message - debug message to print.
 *
 ***********************************************************************/

void KernelPanic(char* message)
{
	KePrint(KERN_CRITICAL "KERNEL PANIC\n");
	KePrint(KERN_CRITICAL "An unrecoverable error has occured. Please restart your computer."
	" If the error occurs again, contact the developers, giving the information on this screen.\n");
	KePrint(KERN_CRITICAL "REASON: %s\n", message);
		
	ThrArchPrintCurrStack();
	
	MachineHalt();
}

SYMBOL_EXPORT(KernelPanic);
