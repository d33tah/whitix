#include <file.h>
#include <stdlib.h>
#include <string.h>
#include <syscalls.h>

#define DIR_BUF_SIZE		512

/* TODO: Use a File* interface when opening/closing? */

struct FileDirectory* FileDirOpen(char* name)
{
	int err;
	struct FileDirectory* ret;

	err=SysOpen(name, _SYS_FILE_FORCE_OPEN | _SYS_FILE_READ | _SYS_FILE_DIRECTORY, 0);
	
	if (err < 0)
		return NULL;

	ret=(struct FileDirectory*)malloc(sizeof(struct FileDirectory));
	ret->fd=err;
	ret->bufSize=DIR_BUF_SIZE;
	ret->buf=(char*)malloc(DIR_BUF_SIZE);
	ret->retVal=(struct FileDirEnt*)malloc(DIR_BUF_SIZE);
	ret->dSize=0;
	ret->next=0;

	return ret;
}

struct FileDirEnt* FileDirNext(struct FileDirectory* dir)
{
	int bytes;
	struct DirEntry* dirEntry;

	if (!dir)
		return NULL;

	if (dir->dSize <= dir->next)
		dir->dSize=dir->next=0;

	if (dir->dSize == 0)
	{
		bytes=SysGetDirEntries(dir->fd, dir->buf, DIR_BUF_SIZE);

		if (bytes <= 0)
			return NULL;

		dir->dSize=bytes;
		dir->next=0;
	}

	dirEntry=(struct DirEntry*)(dir->buf+dir->next);

	/* Zero it out just in case */
	memset(dir->retVal, 0, DIR_BUF_SIZE);
	strcpy(dir->retVal->name, dirEntry->name);

	dir->next+=dirEntry->length;

	return dir->retVal;
}

void FileDirClose(struct FileDirectory* dir)
{
	if (!dir)
		return;

	SysClose(dir->fd);
	free(dir->retVal);
	free(dir->buf);
	free(dir);

	return;
}

static int EntryCompare(const void* a, const void* b)
{
	char** s1 = (char**)a, **s2 = (char**)b;
	
	return strcasecmp(*s1, *s2);
}

int FileDirMatchLoop(struct FileDirEnt* entry, const char* pattern)
{
	char* n = entry->name, *p = pattern;
	
	while (1)
	{
		switch (*p)
		{
			case '*':
			{
				char c;
				
				p++;
				
				/* Check if the next character of the pattern is NULL. If so,
				 * we've successfully matched the pattern */
				if (*p == '\0')
					return 0;
					
				c = *p;
				n = strrchr(n, c);
				if (!n)
					return 1;
					
				break;
			}
				
			default:
				if (*p != *n)
					return 1;
					
				p++; n++;
		}
		
		if (*p == '\0' && *n == '\0')
			return 0;
	}
}

int FileDirMatch(struct FileDirMatches* matches, const char* pattern)
{
	struct FileDirEnt* entry;
	int numFound = 0;
	struct FileDirectory* dir;
	
	struct MatchEntry
	{
		char* name;
		struct MatchEntry* next;
	};
	
	struct MatchEntry* head = NULL;
	char* dirName = NULL, *dirEnd;

	/* Reset the matches structure. */
	matches->numEntries = 0;
	matches->entries = NULL;
	
	/* If dir is NULL, open the current directory or the directory in the pattern.
	 * A common case. */
	dirEnd = strrchr(pattern, '/');
	
	/* No directory name in the pattern? Use the current directory. */
	if (!dirEnd)
	{
		dir = FileDirOpen(".");
		
		if (!dir)
			return -1;
				
		dirName = ".";
	}else{
		dirName = strndup(pattern, dirEnd - pattern+1);
		
		dir = FileDirOpen(dirName);
		
		if (!dir)
			return -1;
			
		pattern = dirEnd+1;
	}
	
	while ((entry = FileDirNext(dir)))
	{
		if (!FileDirMatchLoop(entry, pattern))
		{
			struct MatchEntry* matchEnt, *curr;
			
			matchEnt = (struct MatchEntry*)alloca(sizeof(struct MatchEntry));
			matchEnt->name = strdup(entry->name);
			matchEnt->next = NULL;
			
			if (!head)
				head = matchEnt;
			else
			{
				curr = head;
				
				while (curr->next)
					curr = curr->next;
					
				curr->next = matchEnt;
			}
			
			numFound++;
		}
	}

	if (numFound > 0)
	{
		struct MatchEntry* curr = head;
		int i = 0;
		
		/* We have results. */
		matches->numEntries = numFound;
		matches->entries = (char**)malloc(sizeof(char*) * numFound);
		
		while (curr)
		{
			char* buffer;
			
			if (!strcmp(dirName, "."))
				buffer = strdup(curr->name);
			else{
				buffer = malloc(strlen(dirName) + strlen(curr->name) + 1);
				
				/* TODO: Free the other entries */
				if (!buffer)
					return -1;
				
				strcpy(buffer, dirName);
				strcat(buffer, curr->name);
			}
			
			free(curr->name);
			
			matches->entries[i] = buffer;
			i++;
			
			curr = curr->next;
		}
		
		/* Sort the entries */
		qsort(matches->entries, matches->numEntries, sizeof(char*), EntryCompare);
	}
	
	FileDirClose(dir);

	return 0;	
}
