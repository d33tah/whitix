/* token.c - Library for finding tokens. */

#include <console.h>
#include <stdio.h>
#include <stdlib.h>

#include "internals.h"

void ConsTokenGet(char* string, int i, int* start, int* end)
{
	char* sStart, *sEnd;
	
	if (start)
	{
		sStart=strrchr(&string[i], ' ');
		
		if (sStart)
			*start=(sStart+1)-string;
		else
			*start=0;
	}
	
	if (end)
	{
		sEnd=strchr(&string[i], ' ');
		
		if (sEnd)
			*end=sEnd-string;
		else
			*end=strlen(string);
	}
}

void ConsTokenGetStr(char* string, int i, char** start, char** end)
{
	char* sStart, *sEnd;
	
	if (string[i] == ' ')
		i--;
	
	if (start)
	{
		sStart=&string[i];
		
		while (*sStart != ' ' && sStart > string)
			sStart--;
			
		if (*sStart == ' ')
			sStart++;
			
		*start=sStart;
	}
	
	if (end)
	{
		int len;
		
		len=strlen(string);
		sEnd=&string[i];
		
		while (*sEnd != ' ' && sEnd < string+len)
			sEnd++;
			
		*end=sEnd;
	}	
}
