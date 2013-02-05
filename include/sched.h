/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef SCHED_H
#define SCHED_H

#include <i386/pit.h>
#include <llist.h>
#include <typedefs.h>
#include <string.h>

extern int tasking;
extern volatile int thrNeedSchedule;

struct Process;
struct Thread;
struct KeObject;

extern struct Process* current;
extern struct Thread* currThread,*prevThread;
extern struct Thread* lastMathThread;

int ThrInit();
void ThrSchedule();
struct Process* ThrCreateProcess(char* name);

/* stackP is not applicable to kernel threads. */
#define ThrCreateKernelThread(address) (ThrCreateThread(NULL,(DWORD)(address),false,0, NULL))
#define ThrCreateKernelThreadArg(address, arg) (ThrCreateThread(NULL, (DWORD)(address),	false, 0, (arg)))

struct Thread* ThrCreateThread(struct Process* parent,DWORD entry,int user,DWORD stackP, void* argument);
void ThrStartThread(struct Thread* thread);
void ThrProcessExit(int returnCode);
void ThrSetPriority(struct Thread* thread, int priority);
void ThrSuspendThread(struct Thread* thread);
void ThrResumeThread(struct Thread* thread);
char* ThrGetProcName(DWORD pid);
void ThrFreeThread(struct Thread* thread);
void ThrFreeProcess(struct KeObject* object);
void ThrIdleFunc();
int ThrDoExitThread();

/***********************************************************************
 *
 * PRIORITY DEFINES
 *
 * This is where the scheduler's tweakables are located.
 *
 ***********************************************************************/

#define PRIORITY_NONE		0	/* Lowest priority available */
#define PRIORITY_DEFAULT	16	/* Default priority of a new process */
#define PRIORITY_TOP		32	/* Top priority process, will take up most CPU time */

#endif
