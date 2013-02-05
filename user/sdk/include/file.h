#ifndef _FILE_H
#define _FILE_H

/* Files */
struct File
{
	int fd;
};

#define FILE_READ		1
#define FILE_WRITE		2
#define FILE_APPEND		4

struct File* FileOpen(char* name, int flags);
void FileClose(struct File* file);

/* Directories */

struct FileDirEnt
{
	char name[0];
};

struct FileDirectory
{
	int fd, dSize, next, bufSize;
	char* buf;
	struct FileDirEnt* retVal;
};

struct FileDirectory* FileDirOpen(char* name);
struct FileDirEnt* FileDirNext(struct FileDirectory* dir);
void FileDirClose(struct FileDirectory* dir);

/* Matching */
struct FileDirMatches
{
	int numEntries;
	char** entries;
};

int FileDirMatch(struct FileDirMatches* matches, const char* pattern);
void FileDirMatchClose(struct FileDirMatches* matches);

#endif
