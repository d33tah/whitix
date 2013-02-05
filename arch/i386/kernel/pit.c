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

#include <i386/pit.h>
#include <i386/irq.h>
#include <sched.h>
#include <task.h>
#include <timer.h>
#include <module.h>
#include <time.h>
#include <print.h>

/* Should be exported? */
QWORD time=0;

QWORD PitGetQuantums()
{
	return time;
}

SYMBOL_EXPORT(PitGetQuantums);

int PitInterrupt(void* data)
{
	++time;

	if (currThread)
	{
		if (currThread->quantums > 0)
			--currThread->quantums;
		else
			thrNeedSchedule=true;
	}

	/* Update the timer list */
	UpdateTimers();
	TimeUpdate();

	return IRQ_HANDLED;
}

int PitInit()
{	
	/* Set default setting of 100 hz, so the default timeslice on Whitix is 10ms */
	
	outb(0x43, 0x34);
	outb(0x40, LATCH & 0xFF);
	outb(0x40, LATCH >> 8);

	IrqAdd(0, PitInterrupt, NULL);

	return 0;
}
