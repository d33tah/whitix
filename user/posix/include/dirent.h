#ifndef DIRENT_H
#define DIRENT_H

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>

typedef struct
{
	int fd,dSize,next,bufSize;
	char* buf;
	struct dirent* retVal;
}DIR;

struct dirent
{
	ino_t d_ino;
	char d_name[0];
};

DIR* opendir(const char* dirname);
struct dirent* readdir(DIR* dir);
int closedir(DIR* dir);

#endif
