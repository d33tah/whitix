#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <syscalls.h>

#define _SYS_FILE_READ			1
#define _SYS_FILE_WRITE			2
#define _SYS_FILE_CREATE_OPEN	4
#define _SYS_FILE_FORCE_OPEN	8
#define _SYS_FILE_FORCE_CREATE	16
#define _SYS_FILE_DIRECTORY		32 /* Only open the file if it is a directory */

FILE* fopen(const char* fileName,const char* access)
{
	int flags=0,fd,rw;
	FILE* retVal=NULL;
	char* fName=(char*)fileName;

	if (!fileName || !access)
		return NULL;

	/* Convert the string to permission bits */
	rw=(access[1] == '+') || (access[1] && (access[2] == '+'));
	
	switch (*access)
	{
	case 'a':
		flags=_SYS_FILE_CREATE_OPEN | _SYS_FILE_WRITE;
		break;

	case 'r':
		flags=_SYS_FILE_FORCE_OPEN | (rw ? _SYS_FILE_READ | _SYS_FILE_WRITE : _SYS_FILE_READ);
		break;

	case 'w':
		flags=_SYS_FILE_CREATE_OPEN | (rw ? _SYS_FILE_READ | _SYS_FILE_WRITE : _SYS_FILE_READ);
		break;

	default:
		return NULL;
	}

	fd=SysOpen(fName, flags, 0);
	
	if (fd < 0) /* Error */
	{
		errno=-fd;
		return NULL;
	}
	
	retVal=(FILE*)malloc(sizeof(FILE));
	if (!retVal)
		return NULL;
	
	retVal->fd=fd;
	retVal->flags=0;
	if (*access == 'a')
		fseek(retVal,0,SEEK_END);

	if (*access == 'w')
		SysTruncate(fd,0);

	/* Allocate the I/O buffer */
	retVal->buf=(char*)malloc(BUFSIZ);
	if (!retVal->buf)
	{
		free(retVal);
		return NULL;
	}

	retVal->ptr=retVal->buf;
	retVal->count=0;
	retVal->bufsize=BUFSIZ;
	retVal->position=0;

	return retVal;
}

FILE* fdopen(int fd,const char* access)
{
	FILE* retVal=NULL;
	
	if (fd < 0 || !access)
		return NULL;

	retVal=(FILE*)malloc(sizeof(FILE));
	if (!retVal)
		return NULL;

	retVal->fd=fd;
	retVal->flags=0;
	if (*access == 'a')
		fseek(retVal,0,SEEK_END);

	if (*access == 'w')
		SysTruncate(fd,0);

	/* Allocate the I/O buffer */
	retVal->buf=(char*)malloc(BUFSIZ);
	if (!retVal->buf)
	{
		free(retVal);
		return NULL;
	}

	retVal->ptr=retVal->buf;
	retVal->count=0;
	retVal->bufsize=BUFSIZ;
	retVal->position=0;

	return retVal;
}

FILE* freopen(const char* fileName, const char* access, FILE* stream)
{
	fclose(stream);
	return fopen(fileName, access);
}

int fclose(FILE* file)
{
	if (!file)
		return 0;

	free(file->buf);

	int ret=SysClose(file->fd);
	free(file);
	if (ret < 0) /* Error */
	{
		errno=-ret;
		return EOF;
	}

	return 0;
}

static int FillBuffer(FILE* file)
{
	/* Limit reads to the buffer size */
	int ret=SysRead(file->fd, (char*)file->buf, file->bufsize);
	
	if (ret < 0)
	{
		errno=-ret;
		file->flags |= FILE_ERROR;
		return 1;
	}else if (!ret)
	{
		file->flags |= FILE_EOF;
		file->count=0;
		return 1;
	}

	/* Obviously some bytes have been read, so it's not the EOF */
	file->flags&=~FILE_EOF;

	file->count=ret;
	file->ptr=file->buf;
	return 0;
}

int fread(void* buffer,size_t size,size_t count,FILE* file)
{
	int ret;
	char* p=(char*)buffer;
	size_t totalSize=size*count;

	if (!file)
		return 0;

//	printf("fread(%#X, %u, %u, %d)\n", buffer, size, count, file->fd);

	/* If the file has no buffering, read it all in */
	if (!file->buf)
	{
		ret=SysRead(file->fd,p,totalSize);
		if (ret < 0)
		{
			errno=-ret;
			file->flags |= FILE_ERROR;
			return 0;
		}else if (!ret)
		{
			/* If no bytes are read (but it was successful), it's bound to be an EOF */
			file->flags |= FILE_EOF;
			return 0;
		}

		file->position+=ret;
		return ret/size;
	}else{
		int oldPos=file->position;
		
//		printf("file->count = %u, position = %u, %#X\n", file->count, file->position,
//		file->ptr-file->buf);

		while (totalSize > 0)
		{
			if (file->count < totalSize)
			{
				if (file->count > 0)
				{
					memcpy(p, file->ptr, file->count);
					p+=file->count;
					totalSize-=file->count;
					file->position+=file->count;
				}

				if (FillBuffer(file)) /* FillBuffer handles errors. */
					break;
			}

			if (file->count >= totalSize)
			{
				memcpy(p, file->ptr, totalSize);
				file->ptr+=totalSize;
				file->count-=totalSize;
				file->position+=totalSize;
				return count;
			}
		}

		return (file->position-oldPos)/size;
	}

	return 0;
}

int fwrite(const void* buffer, size_t size, size_t count, FILE* file)
{
	int ret;
	void* buf=(void*)buffer;
	
	if (!file || !size || !count)
	{
		errno=EINVAL;
		file->flags |= FILE_ERROR;
		return 0;
	}

	ret=SysWrite(file->fd, buf, size*count);
	
	if (ret < 0)
	{
		errno=-ret;
		file->flags |= FILE_ERROR;
		return 0;
	}

	file->position+=ret;

	return ret/size;
}

int fseek(FILE* stream,long offset,int whence)
{
	long pos;
	
	if (!stream)
		return -1;
		
	stream->flags &= ~FILE_EOF;

//	printf("fseek(%d, %d, %d)\n", stream->fd, offset, whence);
		
	if (stream->buf && !(stream->flags & _IONBF))
	{
		pos = ftell(stream);
		
		if (whence == SEEK_CUR)
		{
			offset += pos;
			whence = SEEK_SET;
		}
		
		if (whence == SEEK_SET && pos-offset <= stream->ptr - stream->buf
			&& offset-pos <= stream->count)
		{
			stream->ptr += offset-pos;
			stream->count += pos-offset;
			
			stream->position = offset;
			
			return 0;
		}
	}
	
	pos = SysSeek(stream->fd,offset,whence);

	if (pos >= 0)
	{
		stream->position=pos;
		stream->count=0; /* Force fread to re-read buffer. */
		stream->ptr = stream->buf;
	}else{
		errno=-pos;
		stream->flags |= FILE_ERROR;
		return -1;
	}

	return 0;
}

long ftell(FILE* file)
{
	return file->position;
}

int fseeko(FILE* file, off_t offset, int whence)
{
	return fseek(file, offset, whence);
}

off_t ftello(FILE* file)
{
	return ftell(file);
}

int puts(const char* s)
{
	int err=fputs(s,stdout);
	if (err == EOF)
		return err;

	return fputc('\n',stdout);
}

int feof(FILE* file)
{
	if (!file)
		return 1;

	return (file->flags & FILE_EOF);
}

void rewind(FILE* file)
{
	fseek(file,0,SEEK_SET);
	file->flags=0;
}

int remove(const char* filename)
{
	char* fName = (char*)filename;
	
	SysRemove(fName);
	return 0;
}

int unlink(const char *pathname)
{
	SysRemove(pathname);
	return 0;
}

int fflush(FILE* file)
{
	/* Since we don't buffer writes, there's nothing to flush. */
	return 0;
}

void clearerr(FILE* file)
{
	file->flags=0;
}

int ftruncate(int fd, off_t length)
{
	int error=SysTruncate(fd, length);
	if (error < 0)
	{
		errno=-error;
		error=-1;
	}

	return error;
}

/* == TODO == */

int rename(const char* oldpath, const char* newpath)
{
	printf("rename(%s, %s)\n", oldpath, newpath);
	return 0;
}

FILE* tmpfile(void)
{
	printf("tmpfile\n");
	return NULL;
}

FILE* popen(const char* command, const char* type)
{
	printf("popen(%s)\n",command);
	return NULL;
}

int pclose(FILE* file)
{
	printf("pclose\n");
	return 0;
}

int mkstemp(char *template)
{
	printf("mkstemp(%s)\n", template);
	return 0;
}

int __fsetlocking(FILE *stream, int type)
{
	printf("__fsetlocking\n");
	return 0;
}

int ferror_unlocked(FILE *stream)
{
	return ferror(stream);
}

int fgetc_unlocked(FILE* stream)
{
	return fgetc(stream);
}

void clearerr_unlocked(FILE *stream)
{
	printf("clearerr_unlocked\n");
}

size_t fread_unlocked(void* ptr, size_t size, size_t n, FILE* stream)
{
	printf("fread_unlocked\n");
	return 0;
}

size_t fwrite_unlocked(const void* ptr, size_t size, size_t n, FILE* stream)
{
	return fwrite(ptr, size, n, stream);
}

int putc_unlocked(int c, FILE* stream)
{
	return putc(c, stream);
}

/* TODO */
int fputs_unlocked(const char *s, FILE *stream)
{
	return fputs(s, stream);
}

int fflush_unlocked(FILE *stream)
{
	return fflush(stream);
}

int putchar_unlocked(int c)
{
	return putchar(c);
}

int fileno_unlocked(FILE* stream)
{
	return stream->fd;
}
