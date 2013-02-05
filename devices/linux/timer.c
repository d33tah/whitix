#include <console.h>
#include <module.h>
#include <print.h>

void init_timer(void* timer)
{
	KePrint("init_timer\n");
}

SYMBOL_EXPORT(init_timer);

void mod_timer(void* timer, unsigned long expires)
{
	KePrint("mod_timer\n");
}

SYMBOL_EXPORT(mod_timer);

int del_timer_sync(void* timer)
{
	KePrint("del_timer_sync\n");
	return 0;
}

SYMBOL_EXPORT(del_timer_sync);

unsigned long msleep_interruptible(unsigned int msecs)
{
	KePrint("msleep_interruptible\n");
	return 0;
}

SYMBOL_EXPORT(msleep_interruptible);

unsigned long msleep(unsigned int msecs)
{
	KePrint("msleep\n");
	return 0;
}

SYMBOL_EXPORT(msleep);

void __const_udelay(unsigned xloops)
{
	KePrint("__const_udelay\n");
}

SYMBOL_EXPORT(__const_udelay);

void mcount()
{
}

SYMBOL_EXPORT(mcount);

unsigned int jiffies;

SYMBOL_EXPORT(jiffies);
