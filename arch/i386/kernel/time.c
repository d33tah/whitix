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

#include <i386/cmos.h>
#include <i386/sib.h>
#include <console.h>
#include <module.h>
#include <error.h>
#include <sys.h>
#include <time.h>
#include <types.h>
#include <user_acc.h>
#include <sections.h>

struct Time currTime;

SYMBOL_EXPORT(currTime);

/* From Linux - generates a UNIX time in seconds from 1970 */

static time_t TimeMake(unsigned int year,unsigned int month,unsigned int day,
					   unsigned int hour,unsigned int minutes,unsigned int seconds)
{
	if (0 >= (int)(month-=2))
	{
		month+=12;
		year-=1;
	}

	return ((( (time_t)(year/4-year/100+year/400+367*month/12+day)+
		year*365-719499)*24+hour)*60+minutes)*60+seconds;
}

/* FIXME: Need driver for more precise timer. */
void TimeUpdate()
{
	currTime.useconds+=1000000/HZ;
	if (currTime.useconds >= 1000000)
	{
		currTime.useconds=0;
		++currTime.seconds;
	}
	
	SibUpdateTime(currTime.seconds, currTime.useconds);
}

#define BCD_TO_BIN(val)	(((val) & 0x0F)+(((val) >> 4)*10))

initcode static time_t CmosGetTime()
{
	unsigned long year,month,day,hour,minutes,seconds;
	int i;

	for (i=0; i<100000; i++)
		if (CmosRead(RTC_FREQ_SELECT) & RTC_UIP)
			break;

	/* Wait until update in progress flag is not set - so the update is finished */
	for (i=0; i<100000; i++)
		if (!(CmosRead(RTC_FREQ_SELECT) & RTC_UIP))
			break;

	do {
		seconds=CmosRead(CMOS_SECONDS);
		minutes=CmosRead(CMOS_MINUTES);
		hour=CmosRead(CMOS_HOURS);
		day=CmosRead(CMOS_DAY);
		month=CmosRead(CMOS_MONTH);
		year=CmosRead(CMOS_YEAR);
	} while (seconds != CmosRead(CMOS_SECONDS));

	seconds=BCD_TO_BIN(seconds);
	minutes=BCD_TO_BIN(minutes);
	hour=BCD_TO_BIN(hour);
	day=BCD_TO_BIN(day);
	month=BCD_TO_BIN(month);
	year=BCD_TO_BIN(year);

	year+=1900;

	if (year < 1970)
		year+=100;

	return TimeMake(year,month,day,hour,minutes,seconds);
}

initcode int TimeInit()
{
	/* Get the current time from the CMOS, and calculate the UNIX time from it */
	currTime.seconds=CmosGetTime();

#if 0
	/* And get the current useconds offset from the timer (10ms resolution however).
	   Only need lower 32 bits because TimeInit is called soon after the timer is set up.*/
	currTime.useconds=((DWORD)time % (1000/HZ))*HZ;
	printf("useconds = %u\n",currTime.useconds);
#endif

//	SysRegisterCall(SYS_TIME_BASE, &timeSysCall);

	SibUpdateTime(currTime.seconds, 0);

	return 0;
}
