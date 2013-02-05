#ifndef _POSIX_GLOB_H
#define _POSIX_GLOB_H

#include <sys/stat.h>
#include <unistd.h>

typedef struct glob
{
	size_t gl_pathc;
	char** gl_pathv;
	size_t gl_offs;

	/* GNU extensions. Use an ifdef one day? */
	void (*gl_closedir)(void*);
	struct dirent* (*gl_readdir)(void*);
	void* (*gl_opendir)(const char*);
	int (*gl_lstat)(const char*, struct stat*);
	int (*gl_stat)(const char*, struct stat*);
}glob_t;

int glob(const char*, int, int (*)(const char*, int), glob_t* );
void globfree(glob_t*);

#define __ptr_t		void*

/* Flags */
#define GLOB_APPEND			0x01
#define GLOB_DOOFFS			0x02
#define GLOB_ERR			0x04
#define GLOB_MARK			0x08
#define GLOB_NOCHECK		0x10
#define GLOB_NOESCAPE		0x20
#define GLOB_NOSORT			0x40

/* Flags - GNU extensions */
#define GLOB_MAGCHAR		0x80
#define GLOB_ALTDIRFUNC		0x100

#define _GLOB_FLAGS 		(0x1FF)

/* Errors */
#define GLOB_ABORTED		0x01
#define GLOB_NOMATCH		0x02
#define GLOB_NOSPACE		0x03
#define GLOB_NOSYS			0x04

#endif
