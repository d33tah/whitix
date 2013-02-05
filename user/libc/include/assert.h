#ifndef LIBC_ASSERT_H
#define LIBC_ASSERT_H

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) \
	if (!(condition)) DoAssert(__FILE__,__LINE__);
#endif

void DoAssert(char* fileName,int line);

#endif
