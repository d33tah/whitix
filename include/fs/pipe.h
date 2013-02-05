#ifndef FS_PIPE_H
#define FS_PIPE_H

#include <wait.h>

#define PIPE_BUFFER_SIZE	4096

#define PIPE_INFO(file) (((struct PipeInfo*)((file)->vNode->extraInfo)))
#define PIPE_READERS(file) (PIPE_INFO(file)->readers)
#define PIPE_WRITERS(file) (PIPE_INFO(file)->writers)
#define PIPE_FREE(file) (PIPE_BUFFER_SIZE - PIPE_SIZE(file))
#define PIPE_LOCK(file) (PIPE_INFO(file)->lock)
#define PIPE_WAIT(file) (PIPE_INFO(file)->waitQueue)

#define PIPE_SIZE(file) (PIPE_INFO(file)->size)
#define PIPE_EMPTY(file) (PIPE_SIZE(file) == 0)

/* Pipe buffer macros. */
#define PIPE_BUF(file) (PIPE_INFO(file)->base)
#define PIPE_START(file) (PIPE_INFO(file)->start)
#define PIPE_END(file) ((PIPE_START(file) + PIPE_SIZE(file)) & (PIPE_BUFFER_SIZE -1))

#define PIPE_MAX_WCHUNK(file) (PIPE_BUFFER_SIZE - PIPE_END(file))
#define PIPE_MAX_RCHUNK(file) (PIPE_BUFFER_SIZE - PIPE_START(file))

struct PipeInfo
{
	WaitQueue waitQueue;
	char* base;
	unsigned int start;
	unsigned int lock;
	unsigned int readers;
	unsigned int writers;
	unsigned int size;
};

#endif
