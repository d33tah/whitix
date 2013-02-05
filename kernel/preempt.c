#include <preempt.h>
#include <sched.h>
#include <print.h>
#include <task.h>
#include <module.h>

void PreemptDisable()
{
	currThread->preemptCount++;
}

SYMBOL_EXPORT(PreemptDisable);

void PreemptEnable()
{
	currThread->preemptCount--;
	
//	if (!currThread->preemptCount && IrqsEnabled())
//		ThrSchedule();
}

SYMBOL_EXPORT(PreemptEnable);

int PreemptCanSchedule()
{
	return (currThread->preemptCount == 0);
}
