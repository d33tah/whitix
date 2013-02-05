/* 
 * Copyright 2008 Whitix.org. All rights reserved.
 *
 * Distributed under the BSD license. See license.txt in sdk/ for more
 * information.
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

/* Defines */
#define MAX_CONSOLES	7

/* I/O control codes */
#define CONSOLE_SET_OPTIONS 0x00000001
#define CONSOLE_NO_ECHO		0x00000001

#define CONSOLE_GET_OPTIONS 0x00000002

#define CONSOLE_GET_INFO	0x00000003

#define CONSOLE_GET_POSITION	0x00000004

/* VGA color codes. */
#define CONS_COLOR_BLACK		0
#define CONS_COLOR_RED			1
#define CONS_COLOR_GREEN		2
#define CONS_COLOR_YELLOW		3
#define CONS_COLOR_BLUE			4
#define CONS_COLOR_MAGENTA		5
#define CONS_COLOR_CYAN			6
#define CONS_COLOR_WHITE		7

/* Structures */
struct ConsoleInfo
{
	int rows,cols;
};

/* Function prototypes. */
void ConsClearScreen();
void ConsSetForeColor(int color);
void ConsSetBackColor(int color);
void ConsCursorMove(int x,int y);
void ConsInvert();
void ConsColorReset();
void ConsEraseLine();
int ConsGetDimensions(struct ConsoleInfo* consInfo);
int ConsSetEcho(int echoOn);
void ConsCursorMoveRel(int x, int y);
void ConsCursorMoveCol(int col);

/* Line editing */

struct ConsReadContext;

typedef char* (*TabCompleter)(struct ConsReadContext* context, char* string, int state);
typedef void (*TabSetup)(struct ConsReadContext* context, char* string, int start, int end);
typedef int (*BufferUpdate)(char* string, int i, int start, int end);
typedef void (*TabOutput)(char* string);

struct ConsReadContext
{
	char* readBuffer;
	int fd;
	TabCompleter tabCompleter;
	TabSetup tabSetup;
	TabOutput tabOutput;
	
	BufferUpdate bufferUpdate;
};

char* ConsReadLine(struct ConsReadContext* context, char* prompt, int promptColor);
void ConsSetTabCompleter(struct ConsReadContext* context, TabCompleter completer);
void ConsSetTabOutput(struct ConsReadContext* context, TabOutput output);
void ConsSetTabSetup(struct ConsReadContext* context, TabSetup completerSetup);
void ConsSetBufferUpdate(struct ConsReadContext* context, BufferUpdate buffUpdate);
void ConsAddHistory(struct ConsReadContext* context, char* line);
char* ConsGetHistoryEnt();

int ConsFirstToken(char* str, int pos);

void ConsInitContext(struct ConsReadContext* context);

/* Keys. */
#define KEY_DEL 0x7F

#define EXT_BASE 0x80

/* Keypad keys */
#define EXT_UP EXT_BASE+1
#define EXT_DOWN EXT_BASE+2
#define EXT_LEFT EXT_BASE+3
#define EXT_RIGHT EXT_BASE+4

/* Escape key */
#define EXT_ESC EXT_BASE+5

/* Another character comes after this */
#define EXT_CTRL EXT_BASE+0xF

#endif
