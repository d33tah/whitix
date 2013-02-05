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

#ifndef PIT_H
#define PIT_H

#include <typedefs.h>

int PitInit();

QWORD PitGetQuantums();

#define WAIT(time) \
	do{ \
	QWORD orig=PitGetQuantums(); \
	while ((PitGetQuantums()-orig) < time) asm volatile("nop"); \
	}while(0);

#define HZ 100
#define CLOCK_TICK_RATE 1193180 /* The PIT's underlying HZ */
#define LATCH ((CLOCK_TICK_RATE + HZ/2) / HZ)

#endif
