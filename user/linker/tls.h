#ifndef TLS_H
#define TLS_H

struct DynThreadVector
{
};

struct ThreadCb
{
	struct DynThreadVector* vector;
	unsigned long threadId;
};

#define offsetof(type, mem) __builtin_offsetof(type, mem)

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
			

#define THREAD_DTV() \
	( { struct ThreadCb* threadCb; THREAD_GETMEM(threadCb, vector); } )
	
#define GS_SET(seg) \
	 asm ("movw %w0, %%gs" :: "q" (seg) )

#endif
