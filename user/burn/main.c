/* Burn - the Whitix shell */

#include <syscalls.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "builtins.h"

#include <console.h>
#include <file.h>
#include <registry.h>

char prompt[2];
int promptColor = 7;

char currPath[PATH_MAX];
char* currCommand=NULL;

#define MAJOR_BURN_VERSION		0
#define MINOR_BURN_VERSION		2

#define TOKEN_ARRAY_LENGTH		2500

int BurnLineExecute(char** array, int length)
{
	struct CmdTable* curr=btTable;
	struct FileDirectory* cwdir=FileDirOpen(currPath);
	struct FileDirEnt* cwdent;
	struct Stat stat;
	int i;
	
	/* First token should be command */
	if (!array[0])
		return 0; /* Nothing to evaluate */

	while (curr->command)
	{
		if (!strcmp(curr->command, array[0]))
		{
			curr->function(array);
			break;
		}

		curr++;
	}
	
	if (!curr->command)
	{
		/* Try to execute */
		if (BtInExec(array))
		{
			while ( ( cwdent = FileDirNext(cwdir) ) )
			{
				if (cwdent->name[0] == '.')
					continue;
					
				if (SysStat(cwdent->name, &stat, cwdir->fd))
					continue;
					
				if (!(stat.mode & _SYS_STAT_DIR))
					continue;
					
				if (strcasecmp(array[0], cwdent->name) == 0)
					break;
			}
			
			if(cwdent || (strcmp("..", array[0]) == 0))
			{
				char* params[] = {"cd", array[0]};
				BtInChDir(params);
			} else {
				printf("burn: Unknown command or program '%s'.\n\n", array[0]);
				
				/* Add option that defaults to verbose. */
				printf("Your input may not be an application, or may not be installed. Try using "
				"tab completion (see "
				"'help tab') to complete the input, or find more information on executing programs by typing "
				"'help exec'.\n");
			}
		}
	}
	
	FileDirClose(cwdir);
	
	/* Free the tokens */
	for (i=0; i<length; i++)
		free(array[length]);
	
	return 0;
}

void LinePrint(char** array)
{
	while (*array)
	{
		printf("%s ", *array);
		array++;
	}
	
	printf("\n");
}

int BurnExpandLine(char** array, int* length)
{
	int i=0;
	
	while (array[i])
	{
		if (strchr(array[i], '*'))
		{
			struct FileDirMatches matches;
	
			FileDirMatch(&matches, array[i]);
			
			free(array[i]);
			
			/* Move the next arguments over this one (including the one beyond
			 * the end of the array, which will be NULL), then, if there are results,
			 * memmove them back in */

			memmove(&array[i], &array[i+1], (*length - i) * sizeof(char*));
	
			*length += matches.numEntries-1;
	
			if (matches.numEntries > 0)
			{
				int mIndex;
				
//				printf("Here, moving %d to the right\n", length - matches.numEntries);
				
				memmove(&array[i+matches.numEntries], &array[i],
					(*length - matches.numEntries)*sizeof(char*));
				
				for (mIndex = 0; mIndex < matches.numEntries; mIndex++)
				{
					array[i + mIndex] = matches.entries[mIndex];
				}
				
				
			/* We don't want to expand tokens that have appeared as the result
			 * of an expansion (since wildcard characters, like *, aren't allowed
			 * to appear in filenames.
				 */
				i += matches.numEntries-1; /* TODO: Check */
			}

		}else
			i++;
	}

	
	return 0;
}

void BurnStripQuotes(char** string)
{
	char* orig = *string;
	char* str = orig;
	char* new;
	int end = strlen(orig) - 1;
	
	while (*str == '"' || *str == '\'')
		str++;
	
	/* Find the position of the last " or ' */
	while ((orig[end] == '"' || orig[end] == '\'') && end > 0)
		end--;
	
	/* No quotes to strip. */
	if (!end)
		return;	
	
	*string = str;
	
	new = strndup(str, end - (str-orig) + 1);
	
	new[end - (str-orig) + 1 ] = '\0';
	
	/* The original array entry was allocated using strndup, so free it now
	 * and replace it with the new one.
	 */
	 
	 free(orig);
	 
	 *string = new;
}

int ProcessLine(char* buffer)
{
	/* Strtok isn't flexible enough */
	char* array[TOKEN_ARRAY_LENGTH];
	int i=0;
	int j=0;
	
	int quoted = 0;

	int length=strlen(buffer);

	memset(array, 0, TOKEN_ARRAY_LENGTH*sizeof(char*));

	/* Used for tab completion */
	if (currCommand)
	{
		free(currCommand);
	   	currCommand=NULL;
	}
	
	while (i < length)
	{
		int begin;

		/* Skip any whitespace */
		while (isspace(buffer[i]))
			++i;

		if (!quoted)
			begin = i;

		if (buffer[i] == '\0')
		{
			/* End of buffer */
			array[j]=NULL; /* Just null-terminate the array for safety */
			break;
		}
		
		if (j >= TOKEN_ARRAY_LENGTH)
			break;

		while (buffer[i] != '\0' && !isspace(buffer[i]))
		{
			if (buffer[i] == '"')
				quoted = 2 - quoted;
			else if (buffer[i] == '\'')
				quoted = 1 - quoted;
				
			i++;
		}
		
		if (quoted)
			continue;

		/* Terminate the token string. Insert '\0' into the spaces between tokens
		 * in the original string. */
		if (buffer[i] != '\0' && i < length)
		{
			buffer[i]='\0';
			++i;
		}

		array[j++] = strndup(&buffer[begin], i-begin);
	}

	/* quoted should be zero if all the quotes are balanced. */
	if (quoted)
	{
		printf("burn: unterminated double or single quote. Check the format of the command.\n");
		return -EINVAL;
	}
	
	for (i=0; i < j; i++)
		BurnStripQuotes(&array[i]);
	
	BurnExpandLine(array, &j);
	
	return BurnLineExecute(array, j);
}

struct FileDirectory* dir=NULL;
struct FileDirectory* cwdir=NULL;

/* TODO: Needs tidying up and documenting. */
char* CommandCompleter(struct ConsReadContext* context, char* string, int state)
{
	struct FileDirEnt* ent = NULL;
	struct FileDirEnt* cwdent;
	struct Stat stat;
	struct CmdTable* currCmd;
		
	if (!state)
	{
		dir=FileDirOpen("/Applications");
		cwdir=FileDirOpen(currPath);
		
		currCmd = btTable;
		
		if (!dir || !cwdir)
			return NULL;
	}
	
//	printf("%s %s, %d\n", "/applications/", currPath, strcasecmp(currPath, "/applications/"));
	
	if (strcasecmp(currPath, "/applications/"))
	{
		while ( ( ent = FileDirNext(dir) ) )
		{
			if (ent->name[0] == '.')
				continue;
			
			if (SysStat(ent->name, &stat, dir->fd))
				continue;
			
			/* Only files can be executed. */
			if (!(stat.mode & _SYS_STAT_FILE))
				continue;
			
			if (!strncasecmp(string, ent->name, strlen(string)))
				goto out;
		}
	}
	
	while ( ( cwdent = FileDirNext(cwdir) ) )
	{
		if (cwdent->name[0] == '.')
			continue;
			
		if (SysStat(cwdent->name, &stat, cwdir->fd))
			continue;
			
		/* Only files can be executed. */
		if (!(stat.mode & _SYS_STAT_FILE))
			continue;
			
		if (!strncasecmp(string, cwdent->name, strlen(string)))
			goto cwdOut;
	}
	
	while (currCmd->command)
	{
		if (!strncasecmp(string, currCmd->command, strlen(string)))
		{
			char* s=strdup(currCmd->command);
			currCmd++;
			return s;
		}

		currCmd++;
	}
	
	if (!ent && !cwdent && !currCmd)
	{
		FileDirClose(dir);
		FileDirClose(cwdir);
		return NULL;
	}
	
	if(!ent && cwdent)
	{
cwdOut:
		return strdup(cwdent->name);
	}

out:
	return strdup(ent->name);
}

char* ConsFilenameComplete(struct ConsReadContext* context, char* string, int state)
{
	struct FileDirEnt* ent;
	char* name, *entName;
	static char* dirName;

	if (!state)
	{
		char *p;
		
		/* Find the directory name. TODO: Move to library function */
		dirName=(char*)malloc(strlen(string)+2);
		strcpy(dirName, string);
		p=dirName+strlen(dirName);
		
		while (*p != '/' && p > dirName)
			p--;
			
		if (*p == '/')
			p++;
		
		/* Null terminate the string, so we get just the directory name. */	
		*p = '\0';
		
		if (p == dirName)
			strcpy(dirName, ".");
		
		dir=FileDirOpen(dirName);
		
		if (!dir)
			return NULL;
	}
	
	entName=string+strlen(string);
	
	while (*entName != '/' && entName > string)
		entName--;
		
	if (*entName == '/')
		entName++;
	
	while ( ( ent = FileDirNext(dir) ) )
	{
		if (ent->name[0] == '.')
			continue;
			
		if (!strncasecmp(entName, ent->name, strlen(entName)))
			break;
	}
	
	/* We've finished iterating through the directory. Clean up. */
	if (!ent)
	{
		free(dirName);
		FileDirClose(dir);
		return NULL;
	}
	
	name=(char*)malloc(strlen(ent->name)+strlen(dirName)+2);
	name[0]='\0';
	
	/* Should we prefix the directory name? No, if we're completing in the
	 * current directory. */
	if (strlen(dirName) > 1 || dirName[0] != '.')
		strcpy(name, dirName);
	
	strcat(name, ent->name);
	
	struct Stat stat;

	if (!SysStat(ent->name, &stat, dir->fd))
		if (stat.mode & _SYS_STAT_DIR)
			strcat(name, "/");
	
	return name;
}

char* BurnHelpComplete(struct ConsReadContext* context, char* string, int state);
void BurnHelpOutput(char* string);
int BurnHelpTokenUpdate(char* readBuffer, int i, int start, int end);
int BurnCdTokenUpdate(char* readBuffer, int i, int start, int end);

void ConsFilenameOutput(char* str);

void TabCompletionSetup(struct ConsReadContext* context, char* string, int start, int end)
{
	if (!start)
	{
		ConsSetTabCompleter(context, CommandCompleter);
		ConsSetTabOutput(context, ConsFilenameOutput);
	}else{
		if (!strcmp(currCommand, "help"))
		{
			ConsSetTabCompleter(context, BurnHelpComplete);
			ConsSetTabOutput(context, BurnHelpOutput);
		}else
			ConsSetTabCompleter(context, ConsFilenameComplete);
	}
}

int BurnTokenUpdate(char* readBuffer, int i, int start, int end)
{
	char* str=NULL;
	int ret = CONS_COLOR_WHITE;
	
	if (ConsFirstToken(readBuffer, start))
	{
		struct CmdTable* curr;
		struct Stat stat;
		char buf[255];
		
		str=strndup(readBuffer+start, end-start);
		ret=CONS_COLOR_RED;
		
		str[end-start]='\0';
		
		curr=btTable;

		while (curr->command)
		{
			if (!strcmp(str, curr->command))
			{
				ret=CONS_COLOR_GREEN;
				goto out;
			}
		
			curr++;
		}
		
		if (!SysStat(str, &stat, -1) && (stat.mode & _SYS_STAT_FILE))
			goto success;
	
		strcpy(buf, "/Applications/");
		strcat(buf, str);
	
		if (SysStat(buf, &stat, -1) && (stat.mode & _SYS_STAT_FILE))
			goto out;

success:		
		ret=CONS_COLOR_GREEN;
	}else if (currCommand) {	
		/* Get token update function on the first call, and invalidate if copied
		currCommand string differs from currCommand. */
		if (!strcmp(currCommand, "help"))
			ret=BurnHelpTokenUpdate(readBuffer, i, start, end);
		else if (!strcmp(currCommand, "cd"))
		{
			ret=BurnCdTokenUpdate(readBuffer, i, start, end);
		}else
			ret=CONS_COLOR_WHITE;
	}

out:
	if (!start)
	{
		if (currCommand)
			free(currCommand);
		currCommand=str;
	}else if (str)
		free(str);
	
	return ret;
}

int BurnExecuteCommand(char* command)
{
	return ProcessLine(command);
}

struct ConsReadContext context;

int main(int argc, char* argv[])
{
	char* buffer;
	char promptPrint[PATH_MAX];
	
	/* If a -c option is passed, execute the following parameters and quit */
	char* command = NULL;
	int i;
	
	for (i=1; i<argc-1; i++)
		if (!strcmp(argv[i], "-c"))
			command = argv[i+1];
	
	if (command)
		return BurnExecuteCommand(command);
	
    ConsSetForeColor(promptColor);
    
    ConsInitContext(&context);
    ConsSetTabSetup(&context, TabCompletionSetup);
    ConsSetBufferUpdate(&context, BurnTokenUpdate);
    
	printf("Burn shell V%d.%d\n---------------\nType 'help intro' for an introduction to the shell\n",
	 MAJOR_BURN_VERSION, MINOR_BURN_VERSION);
	strcpy(prompt, ">");
	
	/* Get the current directory now, and only update when cd is called */
	SysGetCurrDir(currPath,PATH_MAX);

	while (1)
    {
    	char* save;
    	
    	strcpy(promptPrint, currPath);
    	strcat(promptPrint, prompt);
		
		buffer = ConsReadLine(&context, promptPrint, promptColor);
		
		if (!buffer)
			break;
		
		save = strdup(buffer); /* Better way? */
		
		ProcessLine(buffer);
		ConsAddHistory(&context, save);
		free(save);
	}

	return 0;
}
