#include <stdio.h>
#include <errno.h>

char* fgets(char* s, int n, FILE* file)
{
	char* p;
	int c;
	
	p = s;
	
	while (--n)
	{
		c = fgetc(file);
		
		if (c == EOF)
			break;
			
		if ((*p++ = c) == '\n')
			break;
	}
	
	if (p > s)
	{
		*p = '\0';
		return s;
	}
	
	return NULL;
}

int fgetc(FILE* file)
{
	if (!file)
		return EOF;

	unsigned char c;
	int bytesRead=fread(&c, 1, 1, file);

	if (!bytesRead)
	{
		file->flags |= FILE_EOF;
		return EOF;
	}else if (bytesRead != 1)
	{
		file->flags |= FILE_ERROR;
		errno=-bytesRead;
		return EOF;
	}

	return (int)c;
}

int getc_unlocked(FILE* file)
{
	/* Should be the other way around. */
	return fgetc(file);
}

char* fgets_unlocked(char* s,int size,FILE* file)
{
	printf("fgets_unlocked\n");
	return NULL;
}

int getchar()
{
	return getc(stdin);
}

int ungetc(int c, FILE* file)
{
//	printf("ungetc(%d, %c)\n", file->fd, c);

	if (file->buf && (c != EOF) && (file->ptr > file->buf)
		&& (file->ptr[-1] == (unsigned char)c))
	{
		/* Make it so it is as if the character had never been read. */
		--file->ptr;
		--file->position;
		++file->count;
		file->flags&=~FILE_EOF;
	}else if (file->ptr[-1] != (unsigned char)c)
		c = EOF;
	else{
		printf("ungetc: TODO, c = %#X, buf = %#X\n", c, file->buf);
		exit(0);
	}

	return c;
}
