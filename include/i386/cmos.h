/*  cmos.h - definitions for the CMOS RAM found in PCs.
 *  ===================================================
 *
 *  This file is part of Whitix.
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

#ifndef CMOS_H
#define CMOS_H

#include <i386/ioports.h>
#include <types.h>
#include <typedefs.h>

/* CMOS registers */

#define CMOS_BASE			0x70

#define CMOS_SECONDS		0
#define CMOS_MINUTES		2
#define CMOS_HOURS			4
#define CMOS_WEEK_DAY		6
#define CMOS_DAY			7
#define CMOS_MONTH			8
#define CMOS_YEAR			9

#define RTC_FREQ_SELECT	10
#define RTC_CONTROL		11
#define RTC_IFLAGS		12

/* Register flags */

#define RTC_UIP				0x80

/* Control flags */
#define RTC_PIE				0x40

/* Interrupt flags */
#define RTC_PF				0x40

static inline BYTE CmosRead(BYTE addr)
{
	outb(CMOS_BASE,addr);
	inb(0x80); /* Delay things */
	return inb(CMOS_BASE+1);
}

static inline void CmosWrite(BYTE addr,BYTE val)
{
	outb(CMOS_BASE,addr);
	inb(0x80);
	outb(CMOS_BASE+1,val);
}

#endif
