/* Filesystem syscalls */
SYSCALL(0,	int,	SysOpen,		12,		(char* pathName, int flags, int perms));
SYSCALL(1,	int,	SysCreateDir,	8,		(char* string,int perms)			  );
SYSCALL(2,	int,	SysClose,		4,		(int fd)							  );
SYSCALL(3,	int,	SysRemove,		4,		(char* path)						  );
SYSCALL(4,	int,	SysRemoveDir,	4,		(char* path)						  );
SYSCALL(5,	int,	SysFileAccess,	8,		(char* path,int mode)				  );
SYSCALL(6,	int,	SysFileDup,		4,		(int fd)							  );
SYSCALL(7,int,SysFileSync,4,(int fd));
SYSCALL(8,int,SysFileSystemSync,0,(void));
SYSCALL(9,int,SysTruncate,8,(int fd, unsigned long length));
SYSCALL(10,int,SysMove,8,(char* src,char* dest));
SYSCALL(11,int,SysWrite,12,(int fd,void* data, unsigned long amount));
SYSCALL(12,int,SysRead,12,(int fd,void* buffer,unsigned long amount));
SYSCALL(13,int,SysSeek,12,(int fd,int distance,int whence));
SYSCALL(14,int,SysChangeDir,4,(char* newDir));
SYSCALL(15,int,SysChangeRoot,4,(char* newRoot));
SYSCALL(16,int,SysMount,16,(char* deviceName,char* mountName,char* fsName,void* data));
SYSCALL(17,int,SysUnmount,4,(char* mountPoint));
SYSCALL(18,int,SysGetDirEntries,12,(int fd,void* entries,unsigned long numBytes));
SYSCALL(19,int,SysGetCurrDir,8,(char* path,int len));
SYSCALL(20, int, SysStat, 12, (char* path, void* stat, int fdAt) );
SYSCALL(21, int, SysStatFd, 8, (int fd, void* stat) );
SYSCALL(22,int,SysIoCtl,12,(int fd,unsigned long code,void* data));
SYSCALL(23,int,SysPoll,12,(void* fds, unsigned long numFds, int timeout));
SYSCALL(24,int,SysPipe,4,(int* fds));

/* Memory syscalls */
SYSCALL(25,void*,SysMoreCore,4,(int len));
SYSCALL(26,unsigned long,SysMemoryMap,24,(unsigned long address,unsigned long length,int protection,int fd,unsigned long offset,int flags));
SYSCALL(27,int,SysMemoryProtect, 12, (unsigned long address, unsigned long len, int protection));
SYSCALL(28,int,SysMemoryUnmap,8,(unsigned long address,unsigned long length));
SYSCALL(29,int,SysSharedMemoryGet,12,(unsigned int key, unsigned long size, int flags));
SYSCALL(30,unsigned long,SysSharedMemoryAttach,12,(int id, const void* address, int flags));
SYSCALL(31,int,SysSharedMemoryControl,12,(int id, int command, void* buffer));
SYSCALL(32,int,SysSharedMemoryDetach,4,(const void* address));

/* Task management syscalls */

/* Thread syscalls */
SYSCALL(33, int, SysCreateThread, 12, (unsigned long startAddress,unsigned long stackP, void* argument));
SYSCALL(34, int, SysGetCurrentThreadId, 0, (void));
SYSCALL(35, int, SysSuspendThread, 4, (int));
SYSCALL(36, int, SysResumeThread, 4, (int));
SYSCALL(37, int, SysExitThread, 4, (int thrId));

/* Process syscalls */
SYSCALL(38,int,SysCreateProcess,12,(char* pathName,int* fds,char** argv));
SYSCALL(39,int, SysGetCurrentProcessId, 0, (void));
SYSCALL(40,int,SysExit,4,(int retCode));
SYSCALL(41,int,SysWaitForProcessFinish,8,(int pid,int* finishStatus));

/* Scheduler */
SYSCALL(42,int,SysYield,0,(void));

/* (43) SysGetTime is defined in sib.c */

/* Network syscalls */
SYSCALL(44,int,SysSocketCreate,12,(int domain, int type, int protocol));
SYSCALL(45,int,SysSocketBind,12,(int fd, void* addr, int length));
SYSCALL(46,int,SysSocketConnect,12,(int fd, void* addr, int length));
SYSCALL(47,int,SysSocketListen,8,(int fd, int backlog));
SYSCALL(48,int,SysSocketAccept,12,(int fd, void* addr, int length));
SYSCALL(49,int,SysSocketSend,16,(int fd, const void* buffer, int length, int flags));
SYSCALL(50,int,SysSocketReceive,16,(int fd, void* buffer, int length, int flags));
SYSCALL(51,int,SysSocketIoCtl,12,(int fd, unsigned long code, void* data));
SYSCALL(52,int,SysSocketClose,4,(int fd));

/* Module syscalls */
SYSCALL(53, int, SysModuleAdd, 12, (const char* name, void* data, unsigned int length));
SYSCALL(54, int, SysModuleRemove, 4, (const char* name));

/* Configuration syscalls. */
SYSCALL(55, int, SysConfRead, 12, (char* name, void* data, unsigned int length));
SYSCALL(56, int, SysConfWrite, 12, (char* name, void* data, unsigned int length));

/* Miscellaenous syscalls */
SYSCALL(57,int,SysShutdown,4,(int));
SYSCALL(58,int,SysIoAccess,4,(int on));
SYSCALL(59, int, SysSetThreadArea, 4, (unsigned long base));
