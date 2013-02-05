#include <fs/vfs.h>
#include <print.h>

#include "isofs.h"

DWORD IsoConvertDate(char* date)
{
	int year, month, day, hour, minute, second, i;
	int days;
	DWORD time;

	year=date[0]-70;
	month=date[1];
	day=date[2];
	hour=date[3];
	minute=date[4];
	second=date[5];

	if (year >= 0)
	{
		int monLen[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		days=year*365;
		if (year > 2)
			days += (year+1)/4;

		for (i=1; i<month; i++)
			days+=monLen[i-1];

		if (((year+2) % 4) == 0 && month > 2)
			days++;

		days+=day-1;

		time = ((((days * 24) + hour) * 60 + minute) * 60)+ second;
	}else
		time=0;

	return time;
}
