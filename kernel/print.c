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

#include <config.h>
#include <module.h>
#include <stdarg.h>
#include <string.h>
#include <typedefs.h>

#include <i386/ioports.h>

/* Defines */
#define CONSOLE_OUT_LEN		256

static void (*ConsoleOutput)(char* message, int length) = NULL;

void KeSetOutput(void (*newOutput)(char*, int))
{
	ConsoleOutput=newOutput;
}

SYMBOL_EXPORT(KeSetOutput);

int logLevelUnknown=1;
int currLogLevel=-1;

void KeVaPrint(const char* message, VaList args)
{
	char buf[CONSOLE_OUT_LEN];
	char outputBuf[CONSOLE_OUT_LEN];
	char* start;
	int length=0;
	
	length = vsnprintf(buf, CONSOLE_OUT_LEN, message, args);
	
	if (!length)
		return;
		
	length = 0;

	for (start=buf; *start; start++)
	{
		if (logLevelUnknown)
		{
			/* Parse the beginning of the message to find out the log level. */
			if (*start == '<')
			{
				currLogLevel=*(start+1)-'0';

				/* Skip past the closing > */
				start+=3;
			}else
				currLogLevel=1;

			logLevelUnknown=0;
		}

		if (currLogLevel <= KERN_LOG_LEVEL)
			outputBuf[length++]=*start;

		if (*start == '\n')
			logLevelUnknown=1;
	}

	outputBuf[length]='\0';

	ConsoleOutput(outputBuf, length);
}

SYMBOL_EXPORT(KeVaPrint);

void KePrint(const char* message, ...)
{
	VaList args;
	
	VaStart(args, message);
	KeVaPrint(message, args);
	VaEnd(args);
}

SYMBOL_EXPORT(KePrint);
