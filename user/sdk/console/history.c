/* 
 * Copyright 2008 Whitix.org. All rights reserved.
 *
 * Distributed under the BSD license. See license.txt in sdk/ for more
 * information.
 */

#include <console.h>
#include <stdio.h>
#include <stdlib.h>

#include "internals.h"

struct HistoryEnt
{
	char* line;
	struct HistoryEnt* prev, *next;
};

struct HistoryEnt blank=
{
	.line = "",
	.prev = NULL,
	.next = NULL,
};

struct HistoryEnt* head=NULL, *tail=NULL, *nextEnt=NULL, *curr=&blank;

/* This function is called until it returns NULL. */
char* ConsGetHistoryEnt()
{
	/* we have reached the end of the list */
	if(nextEnt == &blank)
	{
		nextEnt = NULL;
		return NULL;
	}

	/* point nextEnt to the head of the list */
	if(nextEnt == NULL)
	{
		nextEnt = head;
	}

	/* No history. */
	if (!head)
		return NULL;

	char* s = strdup(nextEnt->line);
	nextEnt = nextEnt->next;
	return s;
}

int HistoryDisplay(struct ConsReadContext* context, char* prompt, int dir)
{
	struct HistoryEnt* next;
	char* readBuffer=context->readBuffer;
	
	if (!curr)
		return -1;
	
	if (dir > 0)
		next=curr->next;
	else
		next=curr->prev;
	
	if (!next)
		return -1;
	
	curr=next;
	
	ConsEraseLine();
	ConsCursorMoveRel(-(strlen(readBuffer)+strlen(prompt)), 0);	
	strcpy(readBuffer, curr->line);
	
	fputs(prompt, stdout);
	
	/* TODO: Move to function. */
	int j, start=0;
	
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
	
	return 0;
}

void ConsAddHistory(struct ConsReadContext* context, char* line)
{
	char* tail_line = "";
	struct HistoryEnt* ent;
	
	if (strlen(line) == 0)
		return;
	
	if(tail != NULL)
	{
		tail_line = tail->line;
	}
	
	if(strcmp(line, tail_line)!=0)
	{
		ent=(struct HistoryEnt*)malloc(sizeof(struct HistoryEnt));
		ent->line=strdup(line);
		
		if (head == NULL)
		{
			ent->next=&blank;
			blank.prev=ent;
			ent->prev=NULL;
			head=tail=ent;
		}
		else
		{
			ent->next=&blank;
			blank.prev=ent;
			ent->prev=tail;
			tail->next=ent;
			tail=tail->next;
		}
	}
	
	curr=&blank;
}
