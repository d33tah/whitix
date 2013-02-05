#include <sys/types.h>
#include <dirent.h>
#include <syscalls.h>
#include <errno.h>
#include <stdlib.h>

DIR* opendir(const char* dirName)
{
	int err;
	DIR* ret;

	err=SysOpen(dirName,_SYS_FILE_FORCE_OPEN | _SYS_FILE_READ | _SYS_FILE_DIRECTORY,0);
	
	if (err < 0)
	{
		printf("Could not open %s\n",dirName);
		errno=-err;
		return NULL;
	}

	ret=(DIR*)malloc(sizeof(DIR));
	ret->fd=err;
	ret->bufSize=512;
	ret->buf=(char*)malloc(ret->bufSize);
	ret->retVal=(struct dirent*)malloc(ret->bufSize);
	ret->dSize=0;
	ret->next=0;

	return ret;
}

struct dirent* readdir(DIR* dir)
{
	int bytes;
	struct DirEntry* dirEntry;

	if (!dir)
	{
		errno=EBADF;
		return NULL;
	}

	if (dir->dSize <= dir->next)
		dir->dSize=dir->next=0;

	if (dir->dSize == 0)
	{
		bytes=SysGetDirEntries(dir->fd, dir->buf, 512);

		if (bytes <= 0)
		{
			if (bytes < 0)
				errno=-bytes;
			return NULL;
		}

		dir->dSize=bytes;
		dir->next=0;
	}

	dirEntry=(struct DirEntry*)(dir->buf+dir->next);

	/* Zero it out just in case */
	memset(dir->retVal, 0, 512);

	dir->retVal->d_ino=dirEntry->vNodeNum;
	strcpy(dir->retVal->d_name, dirEntry->name);

	dir->next+=dirEntry->length;

	return dir->retVal;
}

int closedir(DIR* dir)
{
	if (!dir)
	{
		errno=EBADF;
		return -1;
	}

	SysClose(dir->fd);
	free(dir->retVal);
	free(dir->buf);
	free(dir);

	return 0;
}

int chdir(const char* path)
{
	int err;

	err=SysChangeDir(path);

	if (err)
	{
		errno=-err;
		return -1;
	}

	return 0;
}

int fchdir(int fd)
{
	printf("fchdir(%d)\n", fd);
	return 0;
}

int mkdir(const char* pathname, mode_t mode)
{
	int ret = SysCreateDir(pathname, mode);

	if (ret < 0)
	{
		errno = -ret;
		return -1;
	}

	return 0;
}

int rmdir(const char* pathname)
{
	printf("rmdir(%s)\n", pathname);
	return 0;
}

int chroot(const char* path)
{
	printf("chroot\n");
	return 0;
}
int chmod(const char* path, mode_t mode)
{
	printf("chmod\n");
	return 0;
}

int chown(const char* path, uid_t owner, gid_t group)
{
	printf("chown(%s)\n", path);
	return 0;
}

int lchown(const char* path, uid_t owner, gid_t group)
{
	printf("lchown\n");
	return 0;
}

void rewinddir(DIR *dir)
{
	printf("rewinddir\n");
}

char* mkdtemp(char* template)
{
	printf("mkdtemp\n");
	return NULL;
}
