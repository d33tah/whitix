/* tab.c - implementation of tab completion for the console library. */

#include <console.h>
#include <stdio.h>
#include <stdlib.h>

#include "internals.h"

void ConsSetTabCompleter(struct ConsReadContext* context, TabCompleter completer)
{
	context->tabCompleter = completer;
}

void ConsSetTabSetup(struct ConsReadContext* context, TabSetup completerSetup)
{
	context->tabSetup = completerSetup;
}

void ConsSetTabOutput(struct ConsReadContext* context, TabOutput output)
{
	context->tabOutput = output;
}

int stringCompare(const void* p1, const void* p2)
{
	return strcasecmp(*(char**)p1, *(char**)p2);
}

void ConsDoTabComplete(struct ConsReadContext* context, int i, char* prompt, int promptColor)
{
	int total=0, j;
	char* results[TABS_TOTAL];
	char* readBuffer=context->readBuffer;
	char* start, *end;

	ConsTokenGetStr(readBuffer, i, &start, &end);

	/* Let the application look at the read buffer to decide which completer to
	 * use. For example, burn looks at whether the token to be completed is the
	 * first in the string. If it is, it then installs the application completer,
	 * rather than the filename one. */
	if (context->tabSetup != NULL)
		context->tabSetup(context, start, start-readBuffer, end-readBuffer);

	memset(results, 0, TABS_TOTAL*sizeof(char*));
	
	char* tabInput;
	
	tabInput=strndup(start, end-start);

	/* Copy the output from tabCompleter, which consists of strdup'ed strings,
	 * until it returns NULL or we run out of results. */
	while (total < TABS_TOTAL && (results[total]=context->tabCompleter(context, tabInput, total)))
		total++;
		
	if (total > 1)
	{
		/* If there's more than one possible completion, print the results to
		 * the console. */
		fputc('\n', stdout);

		/* Sort into alphabetical order. */
		qsort(results, total, sizeof(char*), stringCompare);

		for (j=0; j<total; j++)
		{
			if (context->tabOutput != NULL)
				context->tabOutput(results[j]);
			else
				puts(results[j]);
		}

#if 0		
		int dist;
		
		/* TODO: MCSS of string. compare to previous string. */
		
		int lCd=dist;
		
		for (j=1; j<total; j++)
		{
			int dist=0;
			char* str=start;
			
			while (results[j][dist++] == *str++);
			
			if (dist < lCd)
				lCd=dist;
		}
		
		printf("lCd = %d\n", lCd);
		
		if (lCd > 0)
			/* TODO: Insert string. */
			strcat(readBuffer, results[0]+strlen(start));
#endif

		/* Render the prompt and readBuffer again. TODO: Call bufferUpdate? */
		 ConsSetForeColor(promptColor);
		fputs(prompt, stdout);
		ConsSetForeColor(CONS_COLOR_WHITE);
		int start=0;
	
		for (j=0; j < strlen(readBuffer); j++)
		{
			if (readBuffer[j] == ' ')
			{
				context->tabSetup(context, readBuffer, start, j-1);
				ConsBufferUpdate(context, j-1, -1);
				putchar(' ');
				start=j+2;
			}
		}

		if (j > 0 && j > start+1)
		{
			context->tabSetup(context, readBuffer, start, j);		
			ConsBufferUpdate(context, j-1, -1);
		}

	}else if (total == 1)
	{
		/* Complete the only result, and render it to the console. */
		char* begin=start+strlen(start);
//		int numChars=strlen(results[0])-(end-start);
		
		/* Insert the results[0]+strlen(start) string at start+strlen(start) */
//		memmove(

//		printf("start = %s, %d, %s\n", start, strlen(start), end);

		/* Move the rest of the string to the right by the number of new characters
		 * from results[0] */
#if 0
		memmove(end+numChars, end, numChars);
		
		printf("numChars = %d, readBuffer = '%s'\n", numChars, readBuffer);
		while (1);
#endif

		strcpy(end-1, results[0]+(end-start-1));
		
		ConsBufferUpdate(context, begin-readBuffer, 1);

		ConsCursorMoveRel(strlen(readBuffer)-(begin-readBuffer), 0);
		
		/* Is there a trailing '/' at the end of the string? If so, it's a
		 * directory, and the user will probably want to tab complete on
		 * subdirectories. */
		if (results[0][strlen(results[0])-1] != '/')
		{
			/* Add a trailing space. */
			start[strlen(start)]=' ';
			putchar(' ');
		}
	}

	/* Free all the strings created by strdup */
	for (j=0; j<total; j++)
		free(results[j]);
		
	free(tabInput);
}

void ConsFilenameOutput(char* str)
{
	char* name=strrchr(str, '/');
	
	if (!name)
		name=str;
	
	if (*name == '/')
		name++;
		
	if (!strlen(name))
	{
		name-=2;
		
		name=memrchr(str,  '/', name-str);
		
		if (!name)
			name=str;
		
		if (*name == '/')
			name++;
	}
	
	puts(name);
}
