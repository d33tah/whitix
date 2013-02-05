#include <fs/icfs.h>
#include <sched.h>
#include <module.h>
#include <slab.h>
#include <task.h>
#include <sys.h>
#include <i386/process.h>
#include <malloc.h>
#include <keobject.h>
#include <user_acc.h>

LIST_HEAD(processList);
extern struct ListHead threadList;
Spinlock procListLock;
struct Cache* processCache;
int pid=0;

struct IcAttribute processAttributes[]=
{
	IC_STRING(struct Process, name, IC_READ), /* Could be RW? */
	IC_INT(struct Process, state, IC_READ),
	IC_INT(struct Process, numFds, IC_READ),
	IC_END(),
};

KE_OBJECT_TYPE(processType, ThrFreeProcess, processAttributes, OffsetOf(struct Process, object));
struct KeSet processSet;

/***********************************************************************
 *
 * FUNCTION: ThrFindProcessById
 *
 * DESCRIPTION: Returns process information for a given process id.
 *
 * PARAMETERS: pid - the id for the process.
 *
 * RETURNS: process with pid's value.
 *
 ***********************************************************************/

struct Process* ThrFindProcessById(DWORD pid)
{
	struct Process* curr;
	int found=0;

	/* 
	 * Don't want a process removed from underneath us while traversing 
	 * the list.
	 */

	PreemptDisable();

	ListForEachEntry(curr,&processList,next)
		if (curr->pid == pid)
		{
			found=1;
			break;
		}

	PreemptEnable();
	
	if (!found)
		return NULL;

	return curr;
}

/***********************************************************************
 *
 * FUNCTION: ThrCreateProcess
 *
 * DESCRIPTION: Create a new user process with the given name. However,
 *		no threads are created with it, so it is not scheduled
 *		until then.
 *
 * PARAMETERS: name - the name of the new process.
 *
 ***********************************************************************/

struct Process* ThrCreateProcess(char* name)
{
	struct Process* process;

	if (!name)
		return NULL;

	process=(struct Process*)MemCacheAlloc(processCache);
	
	if (!process)
		return NULL;

	/* Set up the new process' address space. */
	if (VirtMemManagerSetup(process))
	{
		MemCacheFree(processCache, process);
		
		process = NULL;
		
		goto out;
	}

	/* Allocate the process's name - used when listing processes or when faulting. */
	process->name=(char*)MemAlloc(strlen(name)+1);
	strncpy(process->name, name, strlen(name));

	process->pid = pid++;
	process->cId = 0; /* Used to give IDs to threads. */

	process->state=THR_RUNNING;

	SpinLockIrq(&procListLock);

	/* Add to parent's children list. Used for notify when a parent disappears. */
	process->parent = current;

	if (current)
	{
		ThrGetProcess(process);
		ListAddTail(&process->sibling, &current->children);
	}

	/* Create the process kernel object, and add it to /Processes.
	 * KeObjectInit is called in the slab constructor. */
	if (KeObjectAttach(&process->object, "%d", process->pid))
	{
		ListRemove(&process->sibling);
		MemCacheFree(processCache, process);
		process = NULL;
		goto out;
	}

	ListAddTail(&process->next,&processList);
	
out:

	SpinUnlockIrq(&procListLock);
	return process;
}

SYMBOL_EXPORT(ThrCreateProcess);

/***********************************************************************
 *
 * FUNCTION: ThrFreeProcess
 *
 * DESCRIPTION: Free all resources associated with a process that haven't
 *				already been freed in ThrProcessExit.
 *
 * PARAMETERS: process - the process to be freed.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void ThrFreeProcess(struct KeObject* object)
{
	DWORD flags;
	struct Process* process;
	IrqSaveFlags(flags);

	process = ContainerOf(object, struct Process, object);
	
	PreemptDisable();

	/* Remove process from the parent's list of children. */
	if (process->parent)
		ListRemove(&process->sibling);

	ListRemove(&process->next);

	PreemptEnable();

	MemFree(process->name);
	MemCacheFree(processCache, process);

	IrqRestoreFlags(flags);
}

SYMBOL_EXPORT(ThrFreeProcess);

/***********************************************************************
 *
 * FUNCTION: ThrGetProcName
 *
 * DESCRIPTION: Returns the process name of the given process id.
 *
 * PARAMETERS: pid - the process's id.
 *
 * RETURNS: the process name.
 *
 ***********************************************************************/

char* ThrGetProcName(DWORD pid)
{
	struct Process* curr;
	char* name = "<none>";

	PreemptDisable();

	ListForEachEntry(curr,&processList,next)
		if (curr->pid == pid)
		{
			name = curr->name;
			break;
		}
	
	PreemptEnable();

	return name;
}

/***********************************************************************
 *
 * FUNCTION:	SysWaitForProcessFinish
 *
 * DESCRIPTION:	Waits on a certain process exiting.
 *
 * PARAMETERS:	pid - the process id to wait on.
 *
 * RETURNS:		0 if success, -ETIMEOUT if wait timed out.
 *
 ***********************************************************************/

/* In future, add time to timeout */

int SysWaitForProcessFinish(int pid,int* finishStatus)
{
	struct Process* curr;
	int retVal=0, shouldWait;

	if (finishStatus)
		if (VirtCheckArea(finishStatus,sizeof(int),VER_WRITE))
			return -EFAULT;

repeat:
	PreemptDisable();
	shouldWait = 0;		
	ListForEachEntry(curr, &current->children, sibling)
	{
		if (pid >= 0 && curr->pid != pid)
				continue;
		
		shouldWait = 1;
				
		if (curr->state == THR_DYING)
		{
			if (finishStatus)
				*finishStatus = (curr->exitCode << 8);
				
			retVal = curr->pid;
			PreemptEnable();
			ThrPutProcess(curr);
			goto out;
		}
	}

	PreemptFastEnable();

	if (shouldWait)
	{
//		KePrint("%s, %d: %#X\n", current->name, currThread->id, PreemptCount());
		retVal = 0;
		SLEEP_ON(&current->waitQueue);
		goto repeat;
	}
	
	retVal = -ENOENT;

	KePrint("PREEMPT = %#X\n", currThread->preemptCount);

out:
	return retVal;
}

/***********************************************************************
 *
 * FUNCTION:	SysGetCurrentProcessId
 *
 * DESCRIPTION: Get the pid of the current process.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Process id of the current process.
 *
 ***********************************************************************/

int SysGetCurrentProcessId()
{
	return current->pid;
}

void ThrProcessConstructor(void* obj)
{
	struct Process* process=(struct Process*)obj;

	ZeroMemory(process,sizeof(struct Process));

	INIT_WAITQUEUE_HEAD(&process->waitQueue);
	INIT_LIST_HEAD(&process->areaList);
	INIT_LIST_HEAD(&process->children);
	KeObjectInit(&process->object, &processSet);
}

/***********************************************************************
 *
 * FUNCTION: ThrRelativeAlert
 *
 * DESCRIPTION: Alerts relatives of the process that the process is
 *				exiting.
 *
 * PARAMETERS: None.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

static void ThrRelativeAlert()
{
	struct Process* currProc, *currProc2;

	/* Go through all the children and abandon them. */
	ListForEachEntrySafe(currProc, currProc2, &current->children, sibling)
	{
		ThrPutProcess(currProc);
		currProc->parent=NULL;
	}
	
	/* Signify to parent that process has finished. (if waiting in
	   SysWaitForProcessFinish. */

	if (current->parent)
		WakeUpAll(&current->parent->waitQueue);
}

/***********************************************************************
 *
 * FUNCTION: ThrProcessExit
 *
 * DESCRIPTION: Exit the current process
 *
 * PARAMETERS: returnCode - value to give to parent process.
 *
 * RETURNS: Nothing.
 *
 ***********************************************************************/

void ThrProcessExit(int returnCode)
{
	struct Thread* curr,*curr2;

	SpinLockIrq(&procListLock);

	current->state = THR_DYING;
	current->exitCode = returnCode;

	/* Release all vmm areas. TODO: Should probably do this after other threads have
	 * been ended. */
	MmapProcessRemove(current);
	LoadReleaseFsContext();

	/* Now get rid of the memory manager - leaving kernel memory pages intact */
	VirtDestroyMemManager(current->memManager);
	current->memManager=NULL;

	/* 
	 * Alert the relatives of the process. For example, the parent may be 
	 * waiting in SysWaitForProcessFinish for the process to exit. 
	 */

	ThrRelativeAlert();

	/* Get a reference to the current thread, as it still has to continue on. */
	ThrGetThread(currThread);

	ListForEachEntrySafe(curr,curr2,&threadList,list)
	{
		if (curr->parent == current)
		{
//			if (curr->state != THR_RUNNING)
//				ThrResumeThread(curr); /* FIXME: Need to resume? */
						
			curr->state=THR_DYING;
			
			/* General destruction. */
			TimerThreadExit(curr);
			
			ThrReleaseThread(curr); /* A waitqueue might still have a reference to this thread, so might not be freed */
			ThrArchDestroyThread(curr);
		}
	}

	ThrPutProcess(current);
	
	current = NULL; /* Will reschedule as soon as possible - so no big deal,
					the parent has a reference to the process anyway. */

	SpinUnlockIrq(&procListLock);

	ThrSchedule();
}

SYMBOL_EXPORT(ThrProcessExit);

/***********************************************************************
 *
 * FUNCTION:	SysExit
 *
 * DESCRIPTION: Exits the current process. Doesn't return to
 *				userspace (obviously), but thrNeedSchedule schedules 
 *				afterwards
 *
 * PARAMETERS:	returnCode - the value the current process wants to pass
 *							 to it's parents.
 *
 * RETURNS: 0.
 *
 ***********************************************************************/

int SysExit(int returnCode)
{
	ThrProcessExit(returnCode);
	
	/* Shouldn't get here. */
	return 0;
}

struct SysCall procSysCallTable[]=
{
	SysEntry(SysGetCurrentProcessId, 0),
	SysEntry(SysExit, 4),
	SysEntry(SysWaitForProcessFinish, 8),
	SysEntryEnd()
};

int ProcessInit()
{
	processCache=MemCacheCreate("Processes", sizeof(struct Process), ThrProcessConstructor, NULL, 0);

	SysRegisterRange(SYS_PROCESS_BASE, procSysCallTable);

	/* Create the process set. Exposing the process via the filesystem allows
	 * us to do debugging in quite a nice way. */
	KeSetCreate(&processSet, NULL, &processType, "Processes");

	return 0;
}

