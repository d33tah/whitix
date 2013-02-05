#ifndef PTHREAD_INTERNAL_H
#define PTHREAD_INTERNAL_H

struct ThreadCb
{
	void* p;
	int threadId;
};

#define THREAD_GETMEM(descr, member) \
	( { typeof(descr->member) __value; \
		if (sizeof(__value) == 1) \
			asm volatile("movb %%gs:%P2, %b0" \
				: "=q" (__value) \
				: "0" (0), "i"(offsetof(struct ThreadCb, member))); \
		else if (sizeof(__value) == 4) \
			asm volatile("movl %%gs:%P1, %0" \
				: "=q" (__value) \
				: "i" (offsetof(struct ThreadCb, member))); \
		else \
			asm volatile("movl %%gs:%P1, %%eax\n\t" \
				"movl %%gs:%P2, %%edx" \
				: "=A"(__value) \
				: "i" (offsetof(struct ThreadCb, member)), \
				  "i" (offsetof(struct ThreadCb, member) + 4)); \
		__value; })
		
static inline unsigned long pGetCurrentThreadId()
{
	struct ThreadCb* cb;
	
//	printf("%#X %#X\n", SysGetCurrentThreadId(), THREAD_GETMEM(cb, threadId));
	
	return THREAD_GETMEM(cb, threadId);
//	return SysGetCurrentThreadId();
}
#endif
