#include "builtins.h"

#include <console.h>
#include <errno.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <file.h>

/* Mostly TODO. Output columns for ls. */

struct LsDirEnt
{
	char* name;
};

int LsGetEntries(char* name, struct LsDirEnt** entries, int* maxLen)
{
	struct FileDirectory* dir;
	struct Stat entryStat;
	struct FileDirEnt* ent;
	int numEntries = 0;
	
	/* Start with space for 64 entries, and expand by 64 every time we reach
	 * the end.
	 */
	*entries = (struct LsDirEnt*)calloc(64, sizeof(struct LsDirEnt));

	dir=FileDirOpen(name ? name : ".");
	
	if (dir == NULL)
	{
		puts("ls : Error opening directory");
		return 0;
	}
	
	while ( ( ent = FileDirNext(dir) ) )
	{
		/* Don't display . or .. */
		if (ent->name[0] == '.')
			continue;

		int err=SysStat(ent->name, &entryStat, dir->fd);
		if (err)
			continue;

		if (entryStat.mode & _SYS_STAT_DIR)
			/* Set color to blue for directories. */
			ConsSetForeColor(CONS_COLOR_BLUE);

		printf("%s", ent->name);
		
		if (strlen(ent->name)+2 > *maxLen)
			*maxLen = strlen(ent->name)+2;

		if (entryStat.mode & _SYS_STAT_DIR)
		{
			putchar('/');
			ConsSetForeColor(promptColor);
		}

		putchar('\n');
		
		numEntries++;
	}

	FileDirClose(dir);
	
	return numEntries;
}

int LsOutput(struct LsDirEnt* ent, int numColumns)
{
	return 0;
}

int BtInListDir(char* args[])
{
	int numEntries, maxLen = 0;
	struct LsDirEnt* entries;
	
	numEntries = LsGetEntries(args[1], &entries, &maxLen);

	return 0;
}
