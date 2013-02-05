#include <stdio.h>
#include <errno.h>

int setvbuf(FILE* file,char* buffer,int mode,size_t size)
{
	if ((unsigned int)mode > 2)
	{
		errno=EINVAL;
		return 1;
	}

	/* TODO */
	printf("setvbuf(%d), %#X, mode = %#X, size = %#X\n", file->fd, buffer, mode, size);
	return 0;
}

void setbuf(FILE* file,char* buffer)
{
	setvbuf(file,buffer,((buffer) ? _IOFBF : _IONBF),BUFSIZ);
}
