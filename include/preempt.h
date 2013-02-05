#ifndef PREEMPT_H
#define PREEMPT_H

void PreemptDisable();
void PreemptEnable();

#define PreemptFastEnable() \
	(currThread)->preemptCount--

int PreemptCanSchedule();

#define PreemptCount() (currThread->preemptCount)

#endif
