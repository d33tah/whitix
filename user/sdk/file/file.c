#include <file.h>
#include <stdlib.h>
#include <syscalls.h>

struct File* FileOpen(char* name, int flags)
{
	int fd;
	struct File* ret;
	
	if (!name)
		return NULL;
		
	int openFlags=0;
	
	if (flags & FILE_READ)
		openFlags |= _SYS_FILE_READ | _SYS_FILE_FORCE_OPEN;
		
	if (flags & FILE_WRITE)
		openFlags |= _SYS_FILE_CREATE_OPEN | _SYS_FILE_WRITE | _SYS_FILE_READ;
		
	fd=SysOpen(name, openFlags, 0);
	
	if (fd < 0)
		return NULL;
		
	ret=(struct File*)malloc(sizeof(struct File));
	
	ret->fd=fd;
	
	return ret;
}

int FileGetChar(struct File* file)
{
	if (!file)
		return -1;
		
	unsigned char c;
		
	int bytesRead=SysRead(file->fd, &c, 1);
	
	if (!bytesRead)
		return -1;
	else if (bytesRead != 1)
		return -1;

	return (int)c;
}

int FileReadLine(struct File* file,char* line, unsigned int size)
{
	unsigned int read=0;
	int c;
	
	while (read < size)
	{
		c=FileGetChar(file);
		
		if (c == -1 || c == '\n')
			break;
			
		line[read]=c;
		
		read++;
	}
	
	line[read]='\0';	
	return read;
}

void FileClose(struct File* file)
{
	if (!file)
		return;
		
	SysClose(file->fd);
	free(file);
}
