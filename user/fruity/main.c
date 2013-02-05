#include <console.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <syscalls.h>

#include "keys.h"
#include "file.h"

/* General tweakables */

#define TAB_WIDTH	8

#define MODE_COMMAND 0
#define MODE_INPUT 1

int mode=MODE_INPUT;
int oldFlags;
int cursorY=1;
int linePosition=0;
int cursorX=0; /* This value accounts for tabs */
int maxX,maxY; /* The dimensions of the console */
int isModified=0;

void ConsoleRefresh();
void DoDown();

char* fileName;

#define START_POS(pos) ((pos/maxX)*maxX)

/* Calculates the real length of the string on-screen */
int StrlenTabs(char* str)
{
	int i=0;
	for (; *str; str++)
	{
		if (*str == '\t')
			i+=TAB_WIDTH;
		else
			++i;
	}

	return i;
}

void HeaderSet(char* message)
{
	int x=(maxX/2)-(strlen(message)/2);

	ConsCursorMove(0,0);
	ConsInvert();
	ConsEraseLine();
	ConsCursorMove(x,0);
	fputs(message,stdout);

	ConsColorReset(); /* Set to default */
}

void PromptSet(char* message,...)
{
	va_list args;
	ConsCursorMove(0,maxY-1);
	ConsInvert();
	ConsEraseLine();

	va_start(args, message);
	vprintf(message, args);
	va_end(args);
	ConsColorReset(0);
}

/* Needs cleaning up */

#define QUE_CANCEL 0
#define QUE_YES 1
#define QUE_NO 2

int PromptQuestion(char* message)
{
	PromptSet(message);

	while (1)
	{
		char c=getchar();
		switch (c)
		{
		case 'Y':
		case 'y':
			return QUE_YES;

		case 'N':
		case 'n':
			return QUE_NO;

		case 'C':
		case 'c':
			return QUE_CANCEL;
		}
	}

	return 0;
}

int ConsoleInit()
{
	struct ConsoleInfo conInfo;

	ConsClearScreen();

	/* 
	 * Disable the console echoing input characters, since there's a lot of 
	 * complicated stuff and not all of it should be displayed.
	 */
	ConsSetEcho(0);

	if (ConsGetDimensions(&conInfo))
	{
		printf("fruity: could not get the dimensions of the console.\n");
		exit(0);
	}

	maxX = conInfo.cols;
	maxY = conInfo.rows;
	
	return 0;
}

void DoUp()
{
	int len;

	if (!currLine->prev)
		return;
	
	cursorX=linePosition=0;
	currLine=currLine->prev;

	if (cursorY == 1)
	{
		if (editTop->prev)
			editTop=editTop->prev;
		ConsoleRefresh();
	}else{
		/* Move up several lines if currLine->len > maxX */
		len=currLine->len;
		while (len >= 0)
		{
			len-=maxX;
			cursorY--;
		}
	}

	ConsCursorMove(cursorX,cursorY);
}

void DoLeft()
{
	/* Keypad left */
	if (!linePosition)
	{
		if (currLine == fileStart || !currLine->prev)
			return; /* Not possible, sound an alarm? */
		
		currLine=currLine->prev;
		if (cursorY == 1)
		{
			editTop=editTop->prev;
			ConsoleRefresh();
		}else
			--cursorY;
		
		linePosition=currLine->len;
		cursorX=StrlenTabs(currLine->data+START_POS(linePosition));
	}else if (!cursorX)
	{
		--cursorY;
		cursorX=maxX-1;
		--linePosition;
	}else{
		if (currLine->data[--linePosition] == '\t')
			cursorX-=TAB_WIDTH;
		else
			--cursorX;
	}
	
	ConsCursorMove(cursorX,cursorY);
}

void DoRight()
{
	if (currLine->data[linePosition] == '\t')
	{
		if (maxX-TAB_WIDTH > cursorX)
			cursorX+=TAB_WIDTH;
		else{
			cursorX=TAB_WIDTH-(maxX-cursorX);
			++cursorY;
			++linePosition;
		}
	}else if (currLine->data[linePosition]) 
	/* So at the end of the file, when it can't go down, don't move the cursor right */
		++cursorX;

	if (linePosition >= currLine->len)
		DoDown();
	else if (cursorX >= maxX)
	{
		++cursorY; /* Go down to the next line */
		++linePosition;
		cursorX=0;
	}else
		++linePosition;

	ConsCursorMove(cursorX,cursorY);
}

void DoDown()
{
	int lines;

	if (!currLine->next)
		return;
	
	if (cursorY == maxY-2)
	{
		if (editTop->next)
			editTop=editTop->next;
		ConsoleRefresh(); /* Refresh the whole screen as everything is moving down */
	}else{
		/* Move down several lines if currLine->len > maxX */
		lines=((currLine->len/maxX)-(linePosition/maxX))+1;

		while (lines-- > 0)
			++cursorY;
	}
	
	currLine=currLine->next;
	cursorX = linePosition = 0;

	ConsCursorMove(cursorX, cursorY);
}

#define MIN(a,b) ((a < b) ? a : b)

int LinePrint(struct FileLine* line,int i,int offset)
{
	if (!line)
		return i;

	ConsCursorMove(0,i);
	ConsEraseLine();

	while (offset <= line->len)
	{
		fwrite(line->data+offset, 1, MIN(maxX, line->len-offset), stdout);
		offset+=maxX;

		i++;
		
		if (offset > line->len)
			break;

		if (i >= maxY-1)
			return i;

		ConsCursorMove(0,i);
		ConsEraseLine();
	}

	return i;
}

void ConsoleRefresh()
{
	struct FileLine* curr=editTop;
	int i=1;

	/* File not loaded yet */
	if (!curr)
		return;

	while (i<maxY-1)
	{
		ConsCursorMove(0,i);
		ConsEraseLine();

		if (curr)
		{
			i=LinePrint(curr,i,0);
			curr=curr->next;
		}else
			i++;
	}
}

void DoChar(int c)
{
	int oldLen=currLine->len;

	/* Insert character */
	currLine->data=realloc(currLine->data,currLine->len+2);
	memmove(&currLine->data[linePosition+1], &currLine->data[linePosition], 
		currLine->len-linePosition+1);
	currLine->data[linePosition]=c;
	++currLine->len;

	/* Round down */
	if (currLine->len/maxX > oldLen/maxX)
		ConsoleRefresh();
	else
		/* Just refresh the line(s) */
		LinePrint(currLine, cursorY, START_POS(linePosition));

	DoRight();
	isModified=1;
}

void DoEnter()
{
	/* Splice in between two lines */
	struct FileLine* line=(struct FileLine*)malloc(sizeof(struct FileLine));
	
	line->prev=currLine;
	line->next=currLine->next;
	currLine->next=line;
	
	if (line->next)
		line->next->prev=line;
	else
		fileEnd=line;

	line->data=(char*)malloc(currLine->len-linePosition+1);
	strcpy(line->data,currLine->data+linePosition);

	line->len=currLine->len-linePosition;

	currLine->len=linePosition;
	currLine->data[linePosition]='\0';

	cursorX=linePosition=0;

	if (cursorY == maxY-2)
	{
		editTop=editTop->next;
		ConsoleRefresh();
	}else{
		ConsoleRefresh();
		++cursorY;
	}

	currLine=line;

	ConsCursorMove(cursorX,cursorY);
	++totalLines;
	isModified=1;
}

void DoDelete()
{
	struct FileLine* nextLine;
	int oldLen=currLine->len;

	if (linePosition < currLine->len)
	{
		memmove(&currLine->data[linePosition],&currLine->data[linePosition+1],currLine->len-linePosition);
		--currLine->len;

		/* Number of lines have been changed */
		if (currLine->len/maxX < oldLen/maxX)
			ConsoleRefresh();
		else
			LinePrint(currLine,cursorY,START_POS(linePosition));
	}else if (linePosition == currLine->len && currLine->next)
	{
		nextLine=currLine->next;

		currLine->data=realloc(currLine->data,currLine->len+nextLine->len+1);
		currLine->len=currLine->len+nextLine->len;
		strcat(currLine->data,nextLine->data);
		if (nextLine->next)
			nextLine->next->prev=currLine;

		currLine->next=nextLine->next;

		/* Free the line data - it's not used anymore */
		free(nextLine->data);
		free(nextLine);

		ConsoleRefresh();
	}else
		return;

	ConsCursorMove(cursorX,cursorY);
	isModified=1;
}

void DoBackspace()
{
	struct FileLine* line;

	if (linePosition)
	{
		if (currLine->data[linePosition-1] == '\t')
			cursorX-=TAB_WIDTH;
		else
			--cursorX;

		memmove(&currLine->data[linePosition-1],&currLine->data[linePosition],currLine->len-linePosition+1);
		--linePosition;
		LinePrint(currLine,cursorY,START_POS(linePosition));
	}else{
		/* Delete the current line */
		if (currLine == fileStart)
			return;

		linePosition=currLine->prev->len;
		cursorX=StrlenTabs(currLine->prev->data+START_POS(linePosition)); /* Use rounding to get to nearest start of line, not the best! */
		
		/* Add the old line's contents (or what remains) to this */
		if (currLine->len)
		{
			currLine->prev->data=realloc(currLine->prev->data,currLine->prev->len+currLine->len+2);
			strcat(currLine->prev->data,currLine->data);
			currLine->prev->len+=currLine->len;
		}
		
		currLine->prev->next=currLine->next;

		if (currLine->next)
			currLine->next->prev=currLine->prev;

		if (cursorY == 1)
			editTop=editTop->prev;
		else
			--cursorY;
	
		line=currLine;

		currLine=currLine->prev;
		
		if (line == fileEnd)
		{
			fileEnd=currLine;
			NewBottomLine();
		}

		free(line->data);
		free(line);

		ConsoleRefresh();
	}

	ConsCursorMove(cursorX,cursorY);
	isModified=1;
}

/* Handles ordinary input to the text editor */

int InputHandle(int c)
{
	switch (c)
	{
	/* When the escape key is pressed in input mode, switch to command mode */
	case EXT_ESC:
		mode=MODE_COMMAND;
		ConsCursorMove(0,maxY-1);
		break;

	case EXT_UP:
		DoUp();
		break;

	case EXT_LEFT:
		DoLeft();
		break;

	case EXT_RIGHT:
		DoRight();
		break;

	case EXT_DOWN:
		DoDown();
		break;

	case '\n':
		DoEnter();
		break;

	case '\b':
		DoBackspace();
		break;

	case KEY_DEL:
		DoDelete();
		break;

	case EXT_CTRL:
		mode=MODE_COMMAND;
		break;

	default:
		DoChar(c);
		break;
	}

	return 0;
}

int PromptGetString(char* prompt,char** str)
{
	PromptSet(prompt);
	int c,i=0;

	ConsInvert();

	if (!*str)
		*str=(char*)malloc(maxX-strlen(prompt)+1); /* The maximum that could be typed in */

	while (1)
	{
		c=getchar();
		if (c == '\n')
			break;
		else if (c == EXT_ESC)
			return 1;
		else if (c == '\b')
		{
			if (i)
			{
				(*str)[i]=' ';
				i--;
				putchar(c);
			}
		}else if (c < 0x80)
		{
			(*str)[i++]=(char)c;
			putchar(c);
		}
	}

	(*str)[i]='\0';
	ConsColorReset();

	return 0;
}

void LineGoto()
{
	char* str;
	int lineNo;
	char buf[255];

	sprintf(buf,"Line to go to (1 - %d): ",fileEnd->lineNo+1);

	PromptGetString(buf,&str);

	lineNo=atoi(str);

	if (lineNo < 1 || lineNo > fileEnd->lineNo+1)
	{
		PromptSet("Invalid line number");
		return;
	}

	/* TODO? */
}

/* Returns a value signalling whether the program should quit or not */
int CommandHandle(int c)
{
	switch (c)
	{
	case 'i':
	case 'I':
		break;

	case 'q':
	case 'Q':
		mode=MODE_INPUT;
		return 1;

	case 'S':
	case 's':
		/* Save as the current filename. If there isn't one, ask for it */
		if (!fileName || !strlen(fileName))
		{
			if (PromptGetString("Filename? (ESC to cancel) : ",&fileName))
				goto end;
		}

		if (!SaveFile(fileName))
			PromptSet("Saved '%s'", fileName);
		else
			PromptSet("Failed to save '%s': %s", fileName, strerror(errno));

		break;

	case 'D':
	case 'd':
		if (PromptGetString("FileName? (ESC to cancel) :",&fileName))
			SaveFile(fileName);
		break;

	case 'G':
	case 'g':
		LineGoto();
		break;
	}

	mode=MODE_INPUT;
end:
	ConsCursorMove(cursorX,cursorY);
	return 0;
}

void EditLoop()
{
	int doQuit=0;

	/* Main editing loop */
	while (1)
	{
		int c=getchar();
		
		switch (mode)
		{
		case MODE_INPUT:
			doQuit=InputHandle(c);
			break;

		case MODE_COMMAND:
			doQuit=CommandHandle(c);
			break;
		}

		if (doQuit)
			break;
	}
}

/* Move soon. */
#define PATH_MAX	2048

int main(int argc,char* argv[])
{
	char buf[PATH_MAX+100];
	int ret;

	if (argc >= 2)
	{
		struct Stat stat;

		/* Save off the filename */
		fileName=malloc( MIN(strlen(argv[1])+1, PATH_MAX) );
		if (!fileName)
			return EXIT_FAILURE;
	
		/* Filename has been supplied, so copy it over */
		strncpy(fileName, argv[1], PATH_MAX);

		/* Test whether it is a directory or not. */
		ret=SysStat(fileName, &stat, -1);

		if (ret && ret != -ENOENT)
		{
			printf("fruity: could not get information about the file\n");
			exit(0);
		}

		if (ret == 0)
		{
			if (stat.mode & _SYS_STAT_FILE)
			{
				/* And read it in */
				if (ReadFile(fileName))
					return 0;
			}else{
				printf("fruity: %s: could not open directory.\n", fileName);
				exit(0);
			}
		}else
			NewFile();
	}else
		NewFile();

	if (ConsoleInit())
		return EXIT_FAILURE;

	ConsoleRefresh();

	/* Print the initial prompt message */
	sprintf(buf, "Fruity - editing %s\n", fileName ? fileName : "new file");
	HeaderSet(buf);

	/* For the later question */
	sprintf(buf,"Save %s? (Y = yes, N = no, C = cancel): ",fileName ? fileName : "<untitled>");

start:
	/* And now set up the cursor to 0,1 - the home position */
	ConsCursorMove(0,1);
	cursorX=0;
	currLine=fileStart;

	/* Enter the main editing loop */
	EditLoop();

	if (isModified)
	{
		/* User is now quitting */
		int res=PromptQuestion(buf);

		if (res == QUE_CANCEL)
		{
			ConsEraseLine();
			goto start;
		}

		if (res == QUE_YES)
		{
			if (!fileName)
				if (PromptGetString("FileName? (ESC to cancel) : ",&fileName))
					goto start;

			SaveFile(fileName);
		}
	}

	if (fileName)
		free(fileName);

	/* Leave the console as we found it */
	ConsClearScreen();
	ConsSetEcho(1);

	return 0;
}
