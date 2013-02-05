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

#include <i386/pic.h>
#include <spinlock.h>

/*
 * Must remap hardware IRQs in protected mode, otherwise IRQs will fire a
 * hardware fault/exception.
 */

void PicRemap(int pic1,int pic2)
{
	IrqAccept(15);
	
	/* Send ICW1 - 

	7 || 6 || 5 || 4 || 3 || 2 || 1 || 0
	------------------------------------
	0 || 0 || 0 || 1 || M || 0 || C || I
	
	  M - Indicates the way IRQ 0-7 are activated
	  C - Are the PIC's cascaded?
	  I - Is ICW 4 expected? */
	
	outb(0x20,0x11);
	outb(0xA0,0x11);
	
	/* ICW2 - Send the PIC indices in the IDT */
	outb(0x21,(BYTE)pic1);
	outb(0xA1,(BYTE)pic2);
	
	/* ICW3 - Sets which PIC is master and slave */
	outb(0x21,4);
	outb(0xA1,2);
	
	/* If lowest bit is set, the PIC is working in the x86 arch */
	outb(0x21,0x01);
	outb(0xA1,0x01);
	
	/* Disable all IRQs */
	outb(0x21,0xFF);
	outb(0xA1,0xFF);

	/* Enable slave PIC so interrupts can be redirected. */
	IrqEnable(2);
}

/* Got to serialize access - IrqAccept could be called in the middle of IrqDisable/Enable */

Spinlock picLock;

void IrqAccept(BYTE irq)
{
	SpinLockIrq(&picLock);
	/* Sends an end of interrupt signal so other PIC interrupts can be fired */
	outb(0x20,0x20);
	if (irq >= 8)
		outb(0xA0,0x20);
	SpinUnlockIrq(&picLock);
}
	
/* To enable an IRQ, clear the bit of the IRQ number */
	
void IrqDisable(BYTE irq)
{
	SpinLockIrq(&picLock);

	if(irq < 8)
		outb(0x21,(1<<irq | inb(0x21)));
	else if(irq >= 8 && irq <= 15)
	{
		irq-=8;
		outb(0xA1,(1<<irq | inb(0xA1)));
	}

	SpinUnlockIrq(&picLock);
}

/* To disable an IRQ, set the bit of the IRQ number	*/

void IrqEnable(BYTE irq)
{
	SpinLockIrq(&picLock);

	if(irq < 8)
		outb(0x21,(~(1<<irq)&inb(0x21)));
	else if(irq >= 8 && irq <= 15)
	{
		irq-=8;
		outb(0xA1,(~(1<<irq)&inb(0xA1)));
	}

	SpinUnlockIrq(&picLock);
}
