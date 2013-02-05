/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef VFS_EXPORTS_H
#define VFS_EXPORTS_H

#include <typedefs.h>
#include <types.h>

/* System calls. */

int SysOpen(char* fileName, int flags, int perms);
int SysRead(int fd,BYTE* buffer,DWORD amount);
int SysWrite(int fd,BYTE* data,DWORD amount);
int SysClose(int fd);
int SysFileDup(int fd);
int SysFileSync(int fd);

int SysCreateDir(char* pathName, int flags);
int SysChangeDir(char* dirName);
int SysChangeRoot(char* dirName);
int SysFileSystemSync();
int SysGetDirEntries(int fd,void* entries,size_t count);
int SysSeek(int fd,int distance,int whence);
int SysRemove(char* pathName);

int SysFileAccess(char* path,int mode);
int SysRemoveDir(char* path);
int SysTruncate(int fd,size_t length);
int SysMove(char* src,char* dest);

int SysMount(char* mountPoint,char* deviceName,char* fsName,void* data);
int SysUnmount(char* mountPoint);

int SysChangeDir(char* newDir);
int SysChangeRoot(char* newRoot);

int SysMount(char* mountPoint,char* deviceName,char* fsName,void* data);
int SysUnmount(char* mountPoint);

int SysGetCurrDir(char* path,int len);

int SysStat(char* path, void* stat, int fdAt);
int SysStatFd(int fd, void* buf);

int SysPoll(void* fds, DWORD numFds, int timeout);
int SysPipe(int* fds);

int SysIoCtl(int fd,unsigned long code,char* data);

#endif
