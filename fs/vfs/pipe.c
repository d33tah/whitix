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

#include <console.h>
#include <error.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <task.h>
#include <print.h>
#include <fs/pipe.h>

int PipeRead(struct File* file, BYTE* buffer, DWORD count)
{
	DWORD size = 0, chars = 0;
	int read = 0;
	
	if (!count)
		return 0;
		
	if (file->flags & FILE_NONBLOCK)
	{
		if (PIPE_LOCK(file))
			return 0;
			
//		if (PIPE_EMPTY(file))
//		{
			/* return -EAGAIN? */
//			return 0;
//		}
	}else{

		while (PIPE_EMPTY(file) || PIPE_LOCK(file))
		{
			if (PIPE_EMPTY(file))
				return 0;
	
			KePrint("WAIT\n");			
//			WAIT_ON(&PIPE_WAIT(file), (PIPE_LOCK(file) == 0));
		}
	}
	
	PIPE_LOCK(file)++;
	
	while (count > 0 && (size = PIPE_SIZE(file)))
	{
		char* pipeBuf = PIPE_BUF(file) + PIPE_START(file);
		
		chars = PIPE_MAX_RCHUNK(file);
		
		if (chars > count)
			chars = count;
			
		if (chars > size)
			chars = size;
			
		read += chars;
		
		PIPE_START(file) += chars;
		PIPE_START(file) &= (PIPE_BUFFER_SIZE - 1);
		PIPE_SIZE(file) -= chars;
		count -= chars;
		memcpy(buffer, pipeBuf, chars);
		buffer+=chars;		
	}
	
	PIPE_LOCK(file)--;
	WakeUp(&PIPE_WAIT(file));
	
	return read;
}

int PipeWrite(struct File* file,BYTE* buffer,DWORD size)
{
	DWORD free = 0, chars;
	int written = 0;
	int err = 0;
		
	/* If we've got no readers on this pipe, let userspace know. */
	if (PIPE_READERS(file) == 0)
		return -EPIPE;
		
	if (size <= PIPE_BUFFER_SIZE)
		free = size;
	else
		free = 1;
		
	while (size > 0)
	{
		while ((PIPE_FREE(file) < free || PIPE_LOCK(file)))
		{
			if (!PIPE_READERS(file) == 0)
			{
				err = -EPIPE;
				goto out;
			}
			
			if (file->flags & FILE_NONBLOCK)
			{
				goto out;
			}
			
			WAIT_ON(&PIPE_WAIT(file), PIPE_LOCK(file) == 0);
		}
		
		PIPE_LOCK(file)++;
		
		while (size > 0 && (free = PIPE_FREE(file)))
		{
			char* pipeBuf = PIPE_BUF(file) + PIPE_END(file);
			
			chars = PIPE_MAX_WCHUNK(file);
			
			if (chars > size)
				chars = size;
				
			if (chars > free)
				chars = size;
				
			/* Update counters. */
			written += chars;
			PIPE_SIZE(file) += chars;
			size -= chars;
			
			memcpy(pipeBuf, buffer, chars);
			buffer += chars;
		}
		
		PIPE_LOCK(file)--;
		WakeUp(&PIPE_WAIT(file));
		
		free = 1;
	}
	
out:
	return (written > 0) ? (written) : (err);
}

int PipePoll(struct File* file, struct PollItem* item, struct PollQueue* pollQueue)
{
	int mask = 0;
		
	if (file->vNode->mode & VFS_ATTR_READ)
	{
		if (PIPE_SIZE(file) > 0)
			item->revents = POLL_IN;
	}
	
	if (file->vNode->mode & VFS_ATTR_WRITE)
	{
		if (PIPE_MAX_WCHUNK(file) > 0)
			item->revents = POLL_OUT;
	}
	
	PollAddWait(pollQueue, &PIPE_WAIT(file));
	
	return mask;
}

int PipeClose(struct File* file)
{
	struct PipeInfo* info = (struct PipeInfo*)(file->vNode->extraInfo);

	/* TODO: Free info if there are no readers and no writers. */
	if (file->vNode->mode & VFS_ATTR_READ)
		PIPE_READERS(file)--;
	
	if (file->vNode->mode & VFS_ATTR_WRITE)
		PIPE_WRITERS(file)--;
		
	if (!PIPE_READERS(file) && !PIPE_WRITERS(file))
	{
		/* Remove the pipe. FIXME: Finish. */
		MemFree(info->base);
		MemFree(info);
	}
	
	return 0;
}

struct FileOps readPipeOps=
{
	.read = PipeRead,
	//.write = PipeBadWrite,
	.poll = PipePoll,
	//.seek = PipeBadSeek,
	.close = PipeClose	
};

struct FileOps writePipeOps=
{
	//.read = PipeBadRead
	.write = PipeWrite,
	.poll = PipePoll,
	//.seek = PipeBadSeek,
	.close = PipeClose,
};

/* fds[0] is the read end of the pipe, fd[1] is the write end. */

int SysPipe(int* fds)
{
	int fd1, fd2;
	DWORD flags;
	struct File* firstFile, *secondFile;

	struct PipeInfo* info;
	
	info = (struct PipeInfo*)MemAlloc(sizeof(struct PipeInfo));

	IrqSaveFlags(flags);

	fd1=VfsGetFreeFd(current);

	if (fd1 < 0)
		return -EMFILE;

	firstFile = current->files[fd1] = FileAllocate();
	firstFile->vNode=VNodeGetEmpty();
	firstFile->fileOps=firstFile->vNode->fileOps=&readPipeOps;
	firstFile->vNode->mode = VFS_ATTR_READ | VFS_ATTR_FILE;
	firstFile->vNode->extraInfo = info;

	fd2=VfsGetFreeFd(current);

	if (fd2 < 0)
	{
		/* TODO: Cleanup. */
		return -EMFILE;
	}

	secondFile = current->files[fd2] = FileAllocate();
	secondFile->vNode=VNodeGetEmpty();
	secondFile->vNode->mode = VFS_ATTR_WRITE | VFS_ATTR_FILE;
	secondFile->vNode->extraInfo = info;

	/* Use separate read and write structures. */
	secondFile->fileOps=secondFile->vNode->fileOps=&writePipeOps;

	fds[0]=fd1;
	fds[1]=fd2;
	IrqRestoreFlags(flags);
	
	/* Set up the info structure. */
	info->readers = 1;
	info->writers = 1;
	info->start = 0;
	info->base = (char*)MemAlloc(PIPE_BUFFER_SIZE);
	info->lock = 0;
	INIT_WAITQUEUE_HEAD(&info->waitQueue);

	return 0;
}
