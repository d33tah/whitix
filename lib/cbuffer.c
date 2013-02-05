#include <cbuffer.h>

int CircBufferInit(struct CircBuffer* buffer, int maxSize)
{
	buffer->lock = 0;
	INIT_WAITQUEUE_HEAD(&buffer->waitQueue);
	buffer->base = (char*)malloc(maxSize);
	
	if (!buffer->base)
		return -ENOMEM;
		
	buffer->start = 0;
	buffer->size = 0;
	buffer->maxSize = maxSize;
	
	return 0;
}
