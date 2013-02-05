#ifndef BUILTINS_H
#define BUILTINS_H

/* Used by the shell to execute a command that cannot be found
   in the builtins */
int BtInExec(char* args[]);
int BtInHelp(char* args[]);
int BtInListDir(char* args[]);
int BtInChDir(char* args[]);

struct CmdTable
{
	char* command;
	int (*function)(char* args[]);
};

extern char prompt[2];
extern int promptColor;

extern struct CmdTable btTable[];

#define PATH_MAX 2048
extern char currPath[PATH_MAX];

#endif
