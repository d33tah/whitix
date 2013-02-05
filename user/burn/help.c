#include <syscalls.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "builtins.h"

#include <console.h>
#include <file.h>
#include <registry.h>

struct SectionDesc
{
	char* section, *desc, *text;
};

int currState=0;

#define BOLD	"\33[1m"
#define UNBOLD	"\33[2m"

#define BOLD_TEXT(s) BOLD s UNBOLD

struct SectionDesc sections[]=
{
	/* authors */
	{"authors", "The burn authors", 
		BOLD_TEXT("AUTHORS") "\nMatthew Whitworth\n\n"
		BOLD_TEXT("CONTRIBUTORS") "\nKostadin Damyanov"},
	
	/* burn */	
	{"burn", "About burn",
		BOLD_TEXT("ABOUT BURN")"\n\nBurn is an interactive shell for the Whitix operating system. It features "
		"syntax highlighting, intuitive and detailed error messages and interactive command "
		"completion.\n"},
		
	/* builtins */
	{"builtins", "List of builtin commands",
		"There are a number of builtin commands in burn. Type help 'command' to find out more information about each command:\n\n"
		"\thelp - print this text\n"
		"\tcd - change directory\n"
		"\tls - list the current directory\n"
		"\treboot - reboots the computer\n"
		"\tview - display a file\n"
		"\techo - print text to the screen\n"
		"\tstat - display statistics about a file\n"
		"\tgetcwd - get the current working directory\n"
		"\trm - remove a file\n"
		"\tclear - clears the screen\n"
		"\tprompt - changes the command prompt\n"
		"\tcopy - copies a file from one location to another"},

	/* exec */
	{"exec", "Help on executing commands in burn",
	"Commands are executed by typing \"<program name> <arguments>\" or \"exec <program name> <arguments>\" (if the name of the program is the same as a builtin command of the shell) and pressing enter."},
	{"intro", "Introductory burn tutorial", 
		BOLD_TEXT("WELCOME TO THE BURN SHELL")"\n\nBurn is an interactive shell for the Whitix operating system. "
		"Commands are entered using the keyboard. Type 'help builtins' for a list of builtin commands, and "
		"'search <string>' to search, by description, the list of programs available on Whitix."},
	{NULL, NULL},
};

void BurnHelpOutput(char* string)
{
	struct SectionDesc* curr=sections;
	
	fputs(string, stdout);
	
	while (curr->section)
	{
		if (!strncmp(string, curr->section, strlen(string)))
			break;
			
		curr++;
	}
	
	if (curr->section == NULL)
		goto out;
	
	ConsCursorMoveCol(79); /* End of line. TODO: Not constant */
	ConsCursorMoveRel(-strlen(curr->desc)-2, 0);	
	printf("(%s)", curr->desc);	
		
out:
	putchar('\n');
}

char* BurnHelpComplete(struct ConsReadContext* context, char* string, int state)
{
	if (!state)
		currState=-1;
	
	currState++;
	
	while (sections[currState].section && strncmp(string, sections[currState].section, strlen(string)))
		currState++;
		
	if (!sections[currState].section)
		return NULL;

	return strdup(sections[currState].section);	
}

int BurnHelpTokenUpdate(char* readBuffer, int i, int start, int end)
{
	char* str;
	int ret=CONS_COLOR_RED;
	struct SectionDesc* curr=sections;
	
	str=strndup(readBuffer+start, end-start);
	
	str[end-start]='\0';
	
	while (curr->section && strncmp(readBuffer+start, curr->section, strlen(curr->section)))
	{
		curr++;
	}
	
	if (!curr->section)
		goto out;
		
	ret=CONS_COLOR_CYAN;

out:
	free(str);
	return ret;
}

int BtInHelp(char* args[])
{
	if (!args[1])
	{
		printf("burn help. Type 'help <section>' for more detail.\n");
		struct SectionDesc* curr=sections;
		
		while (curr->section)
		{
			printf("%s - %s\n", curr->section, curr->desc);
			curr++;
		}
		
		return 0;
	}
	
	struct SectionDesc* curr=sections;
	
	while (curr->section && strncmp(args[1], curr->section, strlen(args[1])))
	{
		curr++;
	}
	
	if (!curr->section)
		return 0;
	
	printf("\n%s\n\n", curr->text);

#if 0	
	printf("Burn help\n"
			"help - print this text\n"
			"fruity - basic text editor\n"
			"cd - change directory\n"
			"ls - list the current directory\n"
			"reboot - reboots the computer\n"
			"view - display a file\n"
			"echo - print text to the screen\n"
			"stat - display statistics about a file\n"
			"getcwd - get the current working directory\n"
			"rm - remove a file\n"
			"clear - clears the screen\n"
			"prompt - changes the command prompt\n"
			"copy - copies a file from one location to another\n\n"
			"Use the shift and function keys - together - to switch between different text consoles. Type 'fruity readme.txt'"
			"for a general introduction to the operating system\n");
#endif
	return 0;
}
