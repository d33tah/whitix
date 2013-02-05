#include <sys/file.h>
#include <sys/select.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>

#include <syscalls.h>

/* Sanity check these functions */

int open(const char* path,int oflag,...)
{
	if (!path)
	{
		errno = EFAULT;
		return -1;
	}

	if (oflag & O_EXCL)
	{
		oflag &= ~_SYS_FILE_CREATE_OPEN;
		oflag |= _SYS_FILE_FORCE_CREATE;
		oflag &= ~O_EXCL;
	}

	int fd=SysOpen((char*)path, oflag, 0);
 
	if (fd < 0)
	{
		errno=-fd;
		return -1;
	}

	if (oflag & O_TRUNC)
	{
		int err = SysTruncate(fd, 0);

		if (err < 0)
		{
			SysClose(fd);
			errno = err;
			return -1;
		}
	}
		
	return fd;
}

int creat(const char* path, mode_t mode)
{
	printf("creat\n");
	return 0;
}

ssize_t read(int fd,void* buf,size_t count)
{
	ssize_t ret = SysRead(fd, buf, count);
	
	if (ret < 0)
	{
		errno=-ret;
		return -1;
	}
	
	return ret;
}

ssize_t write(int fd,const void* buf,size_t count)
{
	void* buffer=(void*)buf;
	ssize_t ret = SysWrite(fd, buffer, count);

	if (ret < 0)
	{
		errno=-ret;
		ret=-1;
	}

	return ret;
}

off_t lseek(int fd,off_t offset,int whence)
{
	int ret;
	
	ret = SysSeek(fd, offset, whence);
	
	if (ret < 0)
	{
		errno = -ret;
		ret = -1;
	}
	
	return (off_t)ret;
}

int close(int fd)
{
	return SysClose(fd);
}

char* getcwd(char* buf,size_t size)
{
	int err;

	err=SysGetCurrDir(buf,size);

	if (err < 0)
	{
		errno=-err;
		return NULL;
	}

	return buf;
}

int dup(int fd)
{
	int err=SysFileDup(fd);

	if (err < 0)
	{
		errno=-err;
		return -1;
	}

	return err;
}

/***********************************************************************
 *
 * FUNCTION: CopyStat
 *
 * DESCRIPTION: Translate the native stat structure to a UNIX stat structure
 *
 * PARAMETERS: buf - UNIX stat struct
 *			   theStat - native stat struct
 *
 ***********************************************************************/


void CopyStat(struct stat* buf,struct Stat* theStat)
{
	memset(buf,0,sizeof(struct stat));

	buf->st_ino=theStat->vNum;
	buf->st_mode=theStat->mode;
	buf->st_atime=theStat->aTime;
	buf->st_nlink = 1; /* For now */
	buf->st_mtime=theStat->mTime;
	buf->st_ctime=theStat->cTime;
	buf->st_size=theStat->size;
}

int stat(const char* file_name, struct stat* buf)
{
	int err;
	struct Stat theStat;

	err=SysStat(file_name,&theStat, -1);

	if (err)
	{
		errno=-err;
		return -1;
	}

	CopyStat(buf,&theStat);

	return 0;
}

int lstat(const char* file_name,struct stat* buf)
{
	/* For now */
	return stat(file_name,buf);
}

int fstat(int fd,struct stat* buf)
{
	int err;
	struct Stat theStat;

	err=SysStatFd(fd,&theStat);

	if (err)
	{
		errno=-err;
		return -1;
	}

	CopyStat(buf,&theStat);

	return 0;
}

char* realpath(const char* path, char* resolved_path)
{
	/* Links do not exist yet, so just copy the path. */
	strncpy(resolved_path, path, PATH_MAX);
	return resolved_path;
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
	/* As links do not exist, yet. */
	errno=EINVAL;
	return -1;
}

char* tmpnam(char* s)
{
	printf("tmpnam(%s)\n", s);
	return NULL;
}

char* tmpnam_r(char* s)
{
	return s ? tmpnam(s) : NULL;
}

int fdatasync(int fd)
{
	printf("fdatasync(%d)\n", fd);
	return 0;
}

int fsync(int fd)
{
	printf("fsync(%d)\n", fd);
	return 0;
}

int fcntl(int fd, int cmd, ...)
{
	return O_RDWR;
}

int symlink(const char* oldpath, const char* newpath)
{
	printf("symlink(%s, %s)\n", oldpath, newpath);
	return 0;
}

int link(const char* oldpath, const char* newpath)
{
	printf("link(%s, %s)\n", oldpath, newpath);
	return 0;
}

int access(const char* pathname, int mode)
{
	struct Stat stat;
	
//	printf("access(%s)\n", pathname);
	
	if (SysStat(pathname, &stat, -1))
	{
		errno=ENOENT;
		return -1;
	}
	
	return 0;
}

int mknod(const char* pathname, mode_t mode, dev_t dev)
{
	printf("mknod\n");
	return 0;
}

int mkfifo(const char* pathname, mode_t mode)
{
	printf("mkfifo\n");
	return 0;
}

int pipe(int fds[2])
{
	int ret = SysPipe(fds);

	if (ret)
	{
		errno = -ret;
		ret = -1;
	}

	return ret;
}

int dup2(int oldfd, int newfd)
{
	printf("dup2\n");
	return 0;
}

mode_t umask(mode_t mask)
{
	printf("umask\n");
	return 0;
}

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout)
{
	int i;
	int total=0;
	int outTotal;

//	printf("select(%d, %#X)\n", timeout);

//	if (readfds)
//		printf("read: %u\n", readfds->fd_array[0]);

//	if (writefds)
//		printf("write: %u\n", writefds->fd_array[0]);
 
//	if (timeout)
//		printf("%u %u\n", timeout->tv_sec, timeout->tv_usec);

	/* Construct the poll data structures. First find the number of file
	 * descriptors that are set. */
	for (i=0; i<_FD_MAXIMUM; i++)
	{
		if ((readfds && FD_ISSET(i, readfds)) || (writefds && FD_ISSET(i, readfds))
		|| (exceptfds && FD_ISSET(i, exceptfds)))
			total++;
	}

	if (!total) /* TODO: Fix error. */
		return -1;

	struct pollfd* items=(struct pollfd*)malloc(total*sizeof(struct pollfd));
	int index=0;

	memset(items, 0, total*sizeof(struct pollfd));

	for (i=0; i<_FD_MAXIMUM; i++)
	{
		if (readfds && FD_ISSET(i, readfds))
			items[index].events |= POLLIN;

		if (writefds && FD_ISSET(i, writefds))
			items[index].events |= POLLOUT;
		
		if (exceptfds && FD_ISSET(i, exceptfds))
			items[index].events |= POLLERR;

		if (items[index].events > 0)
		{
			items[index].fd=i;
			index++;
		}
	}

	int err=SysPoll(items, total, timeout->tv_sec); /* TODO: Add nseconds for SysPoll. See select. */
	outTotal=total;

	for (i=0; i<total; i++)
	{
		int fd=items[i].fd;

//		printf("fd = %d, %d, set = %d\n", fd, items[i].revents, FD_ISSET(fd, readfds));

		if (!(items[i].revents & POLLIN) && readfds && FD_ISSET(fd, readfds))
		{
			FD_CLR(fd, readfds);
			outTotal--;
		}

		if (!(items[i].revents & POLLOUT) && writefds && FD_ISSET(fd, writefds))
		{
			FD_CLR(fd, writefds);
			outTotal--;
		}

		if (!(items[i].revents & POLLERR) && exceptfds && FD_ISSET(fd, exceptfds))
		{
			FD_CLR(fd, exceptfds);
			outTotal--;
		}
	}

	return outTotal;
}

void sync(void)
{
	SysFileSystemSync();
}

ssize_t getdelim(char** lineptr, size_t* n, int delimiter, FILE* stream)
{
	char* buf;
	ssize_t pos = -1;
	int c;
	
	if (!lineptr || !n || !stream)
		errno = EINVAL;
	else{
		if (!(buf = *lineptr))
			*n = 0;
			
		pos = 1;
		
		do
		{
			if (pos >= *n)
			{
				if (!(buf = realloc(buf, *n + 64)))
				{
					pos = -1;
					break;
				}
				
				*n += 64;
				*lineptr = buf;
			}
			
			if ((c = fgetc(stream)) != EOF)
			{
				buf[++pos - 2] = c;
				if (c != delimiter)
					continue;
			}
			
			if ((pos -= 2) >= 0)
				buf[++pos] = '\0';
				
			break;
		}while (1);
		
		
	}
	
	return pos;
}

int getline(char** lineptr, size_t* n, FILE* stream)
{
//	printf("*lineptr = %s, %d, %d\n", *lineptr, *n, stream->fd);
	return getdelim(lineptr, n, '\n', stream);
}

int fchown(int fd, uid_t owner, gid_t group)
{
//	printf("fchown\n");
	return 0;
}

int fchmod(int fd, mode_t mode)
{
//	printf("fchmod\n");
	return 0;
}
