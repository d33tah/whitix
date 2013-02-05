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

#ifndef TASK_H
#define TASK_H

#include <llist.h>
#include <typedefs.h>
#include <i386/virtual.h>
#include <i386/i386.h>
#include <wait.h>

struct Process;
struct JournalHandle;

#define THR_USED_MATH 1
#define THR_NEED_RESCHED	2	/* TODO: Use in preemption. */

struct Thread
{
	volatile DWORD currStack;
	struct Process* parent;
	struct Context* currContext;
	DWORD state,entry,esp3;
	struct ListHead list;
	int quantums, flags, priority, id;
	int preemptCount;
	int refs; /* Used by waitqueues and ThrFreeProcess really, otherwise WakeUp ends up waking a thread that doesn't exist */
	union FpuSave fpuData;
	unsigned long tlsGdt[2]; /* for the GDT */
	struct JournalHandle* currHandle; /* Each thread can run its own transaction. */
};

/* FIXME: Move soon. */
inline static void TlsInstall(struct Thread* thread)
{
	extern unsigned char* gdt;
	
	memcpy((((unsigned char*)&gdt)+GDT_TLS_OFFSET), thread->tlsGdt, sizeof(unsigned long)*2);
}



#define ThrGetThread(thread)		((thread)->refs++)
#define ThrReleaseThread(thread) \
do { (thread)->refs--; 	if ((thread)->refs <= 0) ThrFreeThread((thread)); } while(0)

extern struct Thread* idleTask;

/* Thread defines go here */

#define THR_RUNNING 1
#define THR_PAUSED  2
#define THR_DYING   4

struct Process
{
	struct ListHead next,sibling,children;
	int exitCode, state;

	/* File table */
	struct File** files; /* Dynamically allocated */
	Spinlock fileListLock;
	int numFds;

	/* Filesytem context. */
	struct VNode *cwd,*root,*exec;

	char* name;
	WaitQueue waitQueue;
	char* sCwd; /* Allocated dynamically, size is PATH_MAX */
	DWORD pid;
	int cId;
	struct KeObject object;
	struct ListHead areaList;
	struct MemManager* memManager;
	struct Process* parent;
};

#define ThrGetProcess(process) (KeObjGet(&process->object))
#define ThrPutProcess(process) (KeObjPut(&process->object))

#endif
