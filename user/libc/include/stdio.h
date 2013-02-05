#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#define EOF -1
#define FOPEN_MAX 10
#define FILENAME_MAX 255
#define BUFSIZ 2048

#define _IOFBF	0
#define _IOLBF	1
#define _IONBF	2

#ifndef DEFINED_OFFT
#define DEFINED_OFFT
typedef unsigned int off_t;
#endif

#define PRId64	"lld"

/* Private defines for the FILE structure */

#define FILE_EOF 1
#define FILE_ERROR 2

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

typedef struct tagFILE
{
	int fd;
	int flags;
	int position;
	char* buf,*ptr;
	int bufsize;
	int fillsize;
	int count;
	int _lock;
}FILE;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE* fopen(const char* fileName,const char* access);
FILE* freopen(const char* fileName, const char* access, FILE* stream);
int fclose(FILE* file);
int fread(void* buffer,size_t size,size_t count,FILE* file);
int fwrite(const void* buffer,size_t size,size_t count,FILE* file);
int fseek(FILE* stream,long offset,int whence);
FILE* fdopen(int fd,const char* mode);

static inline int ferror(FILE* file)
{
	return (file->flags & FILE_ERROR);
}

#define putc(c,stream) fputc(c,stream)
int putchar(int c);
int puts(const char* s);
int fputs(const char* s,FILE* file);
int fputc(int c,FILE* file);
int printf(const char* str,...);
int vsprintf(char* buf,const char* fmt,va_list args);
int vprintf(const char* str,va_list args);
int vfprintf(FILE* file,const char* format,va_list args);
int fprintf(FILE* file,const char* format,...);
int sprintf(char* buffer,const char* format,...);
int snprintf(char* str,size_t size,const char* format,...);
int vsnprintf(char* str,size_t size,const char* format,va_list ap);
char* fgets(char* s,int size,FILE* file);
int fgetc(FILE* stream);
#define getc(stream) fgetc(stream)
int getc_unlocked(FILE* stream);
int getchar(void);
int sscanf(const char * buf, const char * fmt, ...);
#define fileno(s) ((s)->fd)

int feof(FILE* file);
int fflush(FILE* file);
long ftell(FILE* file);
int remove(const char* filename);
void rewind(FILE* stream);
int setvbuf(FILE* file,char* buffer,int mode,size_t size);
void setbuf(FILE* file,char* buffer);
int rename(const char* old, const char* new);

static inline char* tempnam(const char* dir,const char* pfx)
{
	printf("tempnam\n");
	dir=pfx; /* Temp */
	return NULL;
}

int scanf(const char* format, ...);
int fscanf(FILE* stream, const char* format, ...);

/* POSIX. Move to separate file someday. */
FILE* popen(const char* command, const char* type); /* TODO */
int pclose(FILE* stream);

FILE* tmpfile(void);
char* tmpnam_r(char* s);

#define L_ctermid	32
char* ctermid(char* s);

extern char** sys_errlist;

extern FILE* stdout,*stdin,*stderr;

#endif
