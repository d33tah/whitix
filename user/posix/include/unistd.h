#ifndef UNISTD_H
#define UNISTD_H

/* Includes */
#include <stddef.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdint.h>
#include <limits.h>

#define POSIX

#define _PC_PATH_MAX 2048
#define _POSIX_PATH_MAX	_PC_PATH_MAX
#define PATH_MAX	_PC_PATH_MAX
#define MAXPATHLEN	PATH_MAX

#define _POSIX_THREADS

#define _SC_PAGESIZE	4096
#define _SC_OPEN_MAX	10

long pathconf(char *path, int name);
ssize_t read(int fd,void* buf,size_t count);
ssize_t write(int fd,const void* buf,size_t count);
off_t lseek(int fd,off_t offset,int whence);
int ftruncate(int fd, off_t length);
int close(int fd);
int dup(int fd);
char* getcwd(char* buf,size_t size);
int getpagesize(void);
int unlink(const char* pathname);

int chdir(const char* new_dir);
int access(const char* path, int aMode);

pid_t getppid(void);
pid_t getpid(void);

#define F_OK	0x1
#define R_OK	0x2
#define W_OK	0x4
#define X_OK	0x8

#define S_IRUSR	0x1
#define S_IWUSR 0x2
#define S_IRGRP	0x4
#define S_IWGRP	0x8
#define S_IROTH	0x10
#define S_IWOTH	0x20

/* TTYs, or consoles. */
char* ttyname(int fd);

/* Figure out where these should go. */
int fchdir(int fd);
int chroot(const char* path);
int fsync(int fd);
int fdatasync(int fd);
int chown(const char* path, uid_t owner, gid_t group);
int lchown(const char* path, uid_t owner, gid_t group);
int link(const char* path1, const char* path2);
int nice(int priority);
int rmdir(const char* path);
void _exit(int status);

extern char** environ;
int execv(const char* path, char* const argv[]);
int execve(const char* filename, char* const argv[], char* const envp[]);

uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);

pid_t getpgid(pid_t pid);
pid_t getsid(pid_t pid);
pid_t setsid(void);
int setpgrp(void);
pid_t getpgrp(void);
int setpgid(pid_t pid, pid_t pgid);

char* getlogin(void);

int getgroups(int size, gid_t list[]);
int setgroups(size_t size, const gid_t* list);

/* Users */
int setuid(uid_t uid);
int setgid(gid_t gid);
int seteuid(uid_t uid);
int setegid(gid_t gid);
int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);

ssize_t readlink(const char* path, char* buf, size_t bufsiz);
int symlink(const char* oldpath, const char* newpath);

pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);

int dup2(int oldfd, int newfd);

int isatty(int fd);

int pipe(int fds[2]);
int mkfifo(const char* pathname, mode_t mode);
int mknod(const char* pathname, mode_t mode, dev_t dev);

/* FIX! */
#define major(id) ((id) >> 16)
#define minor(id) ((id) & 0xFFFF)
#define makedev(major, minor) (((major) << 16) | ((minor) & 0xFFFF))

#define L_tmpnam	32

long fpathconf(int fd, int name);

size_t confstr(int name, char *buf, size_t len);
long sysconf(int name);
int getloadavg(double loadavg[], int nelem);

#define PRIO_PROCESS		1

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_int;

#endif
