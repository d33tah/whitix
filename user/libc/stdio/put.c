#include <stdio.h>
#include <errno.h>
#include <syscalls.h>

int fputs(const char* string,FILE* stream)
{
	char* str=(char*)string;
	
	if (!stream)
		return EOF;

	int len=strlen(string);
	stream->position+=len;
	int err=SysWrite(stream->fd, str, len);

	if (err < 0)
	{
		errno=-err;
		return EOF;
	}

	return 1;
}

int fputc(int c,FILE* stream)
{
	if (!stream)
		return EOF;

	char toWrite=*(char*)(&c);

	++stream->position;
	int err=SysWrite(stream->fd,&toWrite,1);
	if (err < 0)
	{
		errno=-err;
		return EOF;
	}

	return c;
}

int putchar(int c)
{
	return fputc(c,stdout);
}

int fputc_unlocked(int c, FILE *stream)
{
	return fputc(c, stream);
}
