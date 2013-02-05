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

#ifndef IRQ_H
#define IRQ_H

#include <console.h>
#include <i386/pic.h>
#include <i386/idt.h>
#include <llist.h>
#include <sched.h>
#include <error.h>

#define MAX_IRQS 16

typedef int (*Irq)(void* data);

struct IrqEntry
{
	Irq irq;
	void* data;
	struct ListHead next;
};

int IrqAdd(BYTE irq,Irq isr,void* data);
int IrqAddEntry(BYTE irq, struct IrqEntry* entry);

int IrqRemove(BYTE irq,Irq isr,void* data);

/* Return codes for interrupt handlers */
#define IRQ_HANDLED		0
#define IRQ_UNHANDLED	1

#endif
