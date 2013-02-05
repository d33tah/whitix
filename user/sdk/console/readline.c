/* 
 * Copyright 2008 Whitix.org. All rights reserved.
 */

#include <console.h>
#include <stdio.h>
#include <stdlib.h>

#include "internals.h"

int ConsFirstToken(char* str, int pos)
{
	while (str[pos] != ' ' && pos > 0)
		--pos;
	
	return (pos == 0);
}

void ConsSetBufferUpdate(struct ConsReadContext* context, BufferUpdate buffUpdate)
{
	context->bufferUpdate = buffUpdate;
}

/* If moveCells is -1, don't move the cursor when rendering the text. (-1 is used when rendering
 * a whole line, like after tab completion)
 */

void ConsBufferUpdate(struct ConsReadContext* context, int cursorPos, int moveCells)
{
	char* readBuffer=context->readBuffer;
	
	if (context->bufferUpdate != NULL)
	{
		int col;
		int start, end;

		start = end = cursorPos;
	
		while (readBuffer[start] != ' ' && start > 0)
			start--;
			
		if (readBuffer[start] == ' ')
			start++;
	
		while (readBuffer[end] != ' ' && readBuffer[end] != '\0')
			end++;

		col=context->bufferUpdate(readBuffer, cursorPos, start, end);
		
		ConsSetForeColor(col);
	
		if (moveCells != -1)
			ConsCursorMoveRel( moveCells - (cursorPos - start ) - 1, 0);
			
		/* Print end-start characters. */
		printf("%.*s", end-start, readBuffer+start);

		ConsSetForeColor(CONS_COLOR_WHITE);
			
		if (moveCells != -1)
			ConsCursorMoveRel(- ( end - cursorPos ) + 1 - moveCells, 0);
	}
}

/* TODO: Move */
#define KEY_DEL		0x7F

char* ConsReadLine(struct ConsReadContext* context, char* prompt, int promptColor)
{
	int i=0, c;
	int length=0;
	char* readBuffer;
	
	ConsSetForeColor(promptColor);
	
	readBuffer=context->readBuffer;

	ConsSetEcho(0);

	memset(context->readBuffer, 0, BUFFER_SIZE);

	fputs(prompt, stdout);

	while (i < BUFFER_SIZE)
	{
		unsigned char ch;
		
		c=getchar();

		if (c == EOF)
			break;

		ch=(unsigned char)c;

		switch (ch)
		{
			case '\0':
				break;
				
			case '\t':
				if (context->tabSetup != NULL || context->tabCompleter != NULL)
				{
					ConsDoTabComplete(context, i, prompt, promptColor);
					length=i=strlen(readBuffer);
				}
				break;

			case EXT_UP:
				HistoryDisplay(context, prompt, -1);
				length=i=strlen(readBuffer);
				break;
				
			case EXT_DOWN:
				HistoryDisplay(context, prompt, 1);
				length=i=strlen(readBuffer);
				break;

			case EXT_LEFT:
				/* Get cursor position, move one to the left. */
				if (i == 0)
					break;
					
				ConsCursorMoveRel(-1, 0);
				i--;
				break;

			case EXT_RIGHT:
				if (i >= length)
					break;
					
				ConsCursorMoveRel(1, 0);
				i++;
				break;
				
			case EXT_CTRL:
				/* Ignore the next character, as we don't do anything special
				 * with the control key pressed.
				 */
				getchar();
				break;

			case '\b':
				if (i == 0)
					break;
					
				i--;
					
				memmove(&readBuffer[i], &readBuffer[i+1], length-(i-1));
					
				if (length - i > 1)
				{
					printf("\33[0K");
					putchar('\b');
					fputs(&readBuffer[i], stdout);
					ConsCursorMoveRel(-(length-i), 0);
				}else{
					readBuffer[length-1]='\0';
					putchar('\b');
				}
				
				length--;
				
				ConsBufferUpdate(context, i, 1);
				break;

			case '\n':
				putchar('\n');
				goto out;
				
			case KEY_DEL:
				if (length == 0 || i == length)
					break;
					
				memmove(&readBuffer[i], &readBuffer[i+1], length-(i-1));
				
				printf("\33[0K");
				fputs(&readBuffer[i], stdout);
				ConsCursorMoveRel(-(length-i)+1, 0);
				
				if (readBuffer[i] != ' ')
					ConsBufferUpdate(context, i, 1);
					
				length--;
				
				break;

			default:
			{
				int toRight=length-i;
				memmove(&readBuffer[i+1], &readBuffer[i], toRight+1);
				readBuffer[i]=c;
				length++;
				
				if (toRight)
				{
					printf("\33[0K");
					fputs(&readBuffer[i], stdout);
					ConsCursorMoveRel(-toRight, 0);
				}else
					putchar(c);
				
				if (c != ' ')
					ConsBufferUpdate(context, i, 0);
				
				i++;
			}
				
		}
	}

out:
	ConsSetEcho(1);
	readBuffer[length]='\0';

	if (length == 0 && c == EOF)
		return NULL;
	
	ConsSetForeColor(CONS_COLOR_WHITE);

	return readBuffer;
}

void ConsInitContext(struct ConsReadContext* context)
{
	memset(context, 0, sizeof(struct ConsReadContext));
	
	context->readBuffer=(char*)malloc(BUFFER_SIZE);
	context->fd=1;
	context->tabOutput=ConsFilenameOutput;
}

void ConsFreeContext(struct ConsReadContext* context)
{
	free(context->readBuffer);
}
