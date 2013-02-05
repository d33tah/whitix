#ifndef LOCKS_H
#define LOCKS_H

#ifndef SMP
#define PREFIX_LOCK		"lock; "
#else
#define PREFIX_LOCK	""
#endif

#define ADDR (*(volatile long*)address)

static inline int BitTest(volatile void* address, int bit)
{
	int oldBit;

	asm volatile(""
		"btl %2, %1\n\t"
		"sbbl %0, %0"
		: "=r"(oldBit)
		: "m"(ADDR), "Ir"(bit));

	return oldBit;
}

static inline void BitSet(volatile void* address, int bit)
{
	asm volatile(""
		"btsl %1, %0"
		:"=m"(ADDR)
		:"Ir"(bit));
}

static inline void BitClear(volatile void* address, int bit)
{
	asm volatile(""
		"btrl %1, %0"
		:"+m"(ADDR)
		:"Ir"(bit));
}

static inline int BitTestAndSet(volatile void* address, int bit)
{
	int oldBit;

	asm volatile(""
		"btsl %2, %1\n\t"
		"sbbl %0, %0"
		:"=r"(oldBit), "=m"(ADDR)
		:"ir"(bit) : "memory");

	return oldBit;
}

#endif
