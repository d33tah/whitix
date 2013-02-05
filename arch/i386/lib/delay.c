#include <delay.h>
#include <module.h>
#include <print.h>
#include <typedefs.h>
#include <i386/pit.h>

DWORD uDelayLoops;

#define rdtscl(low) \
	asm volatile("rdtsc" : "=a"(low) :: "edx")

static void RdtscDelay(unsigned long loops)
{
	unsigned beginClock, now;
	
	rdtscl(beginClock);
	
	do
	{
		asm volatile("rep; nop");
		rdtscl(now);
	}while ((now-beginClock) < loops);
}

void uDelayConst(unsigned long xLoops)
{
	int out;
	
	asm volatile("mull %0"
			: "=d" (xLoops), "=&a"(out)
			: "1"(xLoops), "0"(uDelayLoops));
			
	RdtscDelay(xLoops * 100);
}

void uDelay(unsigned long usecs)
{
	return uDelayConst(usecs * 0x000010C6);
}

SYMBOL_EXPORT(uDelay);

#define currTicks (unsigned long)PitGetQuantums()

int DelayInit()
{
	unsigned long ticks, loopBit;
	int precision = 8;
	
	sti();
	
	KePrint(KERN_INFO "CPU: calibrating delay loop\n");
	
	uDelayLoops = (1 << 12);
	
	while ((uDelayLoops <<= 1))
	{
		ticks = currTicks;
		while (ticks == currTicks);
		ticks = currTicks;
		uDelay(uDelayLoops);
		ticks = currTicks - ticks;
		
		if (ticks)
			break;
	}
	
	uDelayLoops >>= 1;
	
	loopBit = uDelayLoops;
	
	while (precision-- && (loopBit >>= 1))
	{
		uDelayLoops |= loopBit;
		ticks = currTicks;
		while (ticks == currTicks);
		ticks = currTicks;
		uDelay(uDelayLoops);
		if (ticks != currTicks)
			/* Longer than one tick? */
			uDelayLoops &= ~loopBit;
	}
	
	return 0;
}
