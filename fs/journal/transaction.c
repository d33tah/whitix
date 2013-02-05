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

#include <fs/journal.h>
#include <fs/vfs.h>
#include <module.h>
#include <print.h>
#include <slab.h>

extern struct Cache* journHandleCache, *journTransCache;

/*******************************************************************************
 *
 * FUNCTION: 	JournalTransCreate
 *
 * DESCRIPTION: Create a new current transaction for the journal. The journal
 *				lock must be held on entry to this function. Create it in a
 *				RUNNING state and add it to the new journal.
 *
 * PARAMETERS: 	journal - the journal to create a new filesystem transaction for
 *
 * RETURNS: 	a structure representing the new current transaction for the
 *				journal.
 *
 ******************************************************************************/

struct JournalTrans* JournalTransCreate(struct Journal* journal)
{
	struct JournalTrans* ret;

	ret=(struct JournalTrans*)MemCacheAlloc(journTransCache);

	if (!ret)
		return NULL;

	ret->journal=journal;
	ret->state=JTRANS_RUNNING;
	ret->transId=journal->transSequence++;
	ret->expires=currTime.seconds+JOURN_COMMIT_INTERVAL;

	/* TODO: Move to constructor. */
	INIT_LIST_HEAD(&ret->reservedList);
	INIT_LIST_HEAD(&ret->metaDataList);
	INIT_LIST_HEAD(&ret->logControlList);
	INIT_LIST_HEAD(&ret->shadowList);
	INIT_LIST_HEAD(&ret->ioList);
	INIT_LIST_HEAD(&ret->forgetList);

	/* TODO: Set up timer. */

	journal->currTransaction=ret;

	return ret;
}

int JournalHandleStart(struct Journal* journal, struct JournalHandle* handle)
{
	DWORD numBlocks=handle->numBlocks;
	struct JournalTrans* trans;
	DWORD needed;

	if (numBlocks > journal->maxTransactionBuffers)
	{
		KePrint(KERN_INFO "JournalHandleStart: handle wants too many credits (%d, "
		"%d)\n", numBlocks, journal->maxTransactionBuffers);
		return -ENOSPC;
	}

	JournalLock(journal);

	/* Check if journal is aborted. */

	/* Wait on the journal's transaction barrier. */

	/* Create the transaction. */
	if (!journal->currTransaction)
		journal->currTransaction=JournalTransCreate(journal);

	trans=journal->currTransaction;

	/* Check if locked. */

	needed=trans->outstandingBlocks+numBlocks;

	if (needed > journal->maxTransactionBuffers)
	{
		/* Current transaction is too large. TODO: Commit it. */
	}

	/* Check log space left. */
//	KePrint("free = %u\n", journal->free);

	/* Account for all the buffers, and add the handle to the running transaction. */
	handle->transaction=trans;
	trans->outstandingBlocks+=numBlocks;
	trans->updates++;
	trans->handleCount++;

	JournalUnlock(journal);

	return 0;
}

struct JournalHandle* JournalHandleAllocate(int numBlocks)
{
	struct JournalHandle* ret=(struct JournalHandle*)MemCacheAlloc(journHandleCache);

	if (!ret)
		return NULL;

	ret->refs=1;
	ret->numBlocks=numBlocks;

	return ret;
}

struct JournalHandle* JournalStart(struct Journal* journal, int numBlocks)
{
	struct JournalHandle* handle=JournalCurrHandle();
	int ret;

	if (handle)
	{
		handle->refs++;
		return handle;
	}
	
	JournalLock(journal);

	handle=JournalHandleAllocate(numBlocks);
	currThread->currHandle=handle;
	ret=JournalHandleStart(journal, handle);

	JournalUnlock(journal);

	if (ret)
	{
		/* JournalHandleFree */
		currThread->currHandle=NULL;
		return NULL;
	}

	return handle;
}

SYMBOL_EXPORT(JournalStart);

int JournalStop(struct JournalHandle* handle)
{
	struct JournalTrans* trans=handle->transaction;
	struct Journal* journal=trans->journal;
	
	currThread->currHandle=NULL;

	--handle->refs;
	
	if (handle->refs > 0)
		return 0;

	trans->updates--;
	trans->outstandingBlocks-=handle->numBlocks;

//	KePrint("JournalStop: %u\n", trans->updates);

	if (!trans->updates)
	{
//		KePrint("Here! %u %u\n", currTime.seconds, trans->expires);
		WakeUp(&journal->waitUpdates);
	}

	if (handle->sync == 1 || currTime.seconds >= trans->expires)
	{
		int id=trans->transId;

		JournalStartCommit(journal, trans);

		if (handle->sync == 1)
			JournalWaitCommit(journal, id);
	}

	MemCacheFree(journHandleCache, handle);

	return 0;
}

SYMBOL_EXPORT(JournalStop);

void JournalFileBuffer(struct JournalHead* head, struct JournalTrans* trans, int type)
{
	struct ListHead* list;

	if (!head)
		return;

	/* Already filed? */
	if (head->transaction && head->list == type)
		return;
		
	PreemptDisable();

	/* 
	 * It's already in this transaction, but on a different list, so we remove it
	 * before adding it to the other list.
	 */

	if (head->transaction)
		JournalUnfileBuffer(head);
	else
		head->transaction=trans;

	switch (type)
	{
		case JOURN_RESERVED:
			list=&trans->reservedList;
			break;

		case JOURN_METADATA:
			list=&trans->metaDataList;
			break;

		case JOURN_LOGCTL:
			list=&trans->logControlList;
			break;

		case JOURN_SHADOW:
			list=&trans->shadowList;
			break;

		case JOURN_IO:
			list=&trans->ioList;
			break;
			
		case JOURN_FORGET:
			list=&trans->forgetList;
			break;

		default:
			KePrint("FileBuffer: handle %d!\n", type);
			return;
	}

	ListAddTail(&head->next, list);
	head->list=type;
	
	PreemptEnable();
}

void JournalUnfileBuffer(struct JournalHead* head)
{
	ListRemove(&head->next);
}

/* Intent to modify a buffer for metadata update. */

int JournalGetWriteAccess(struct JournalHandle* handle, struct Buffer* buffer)
{
	struct JournalTrans* trans=handle->transaction;
	struct Journal* journal=trans->journal;
	struct JournalHead* head;

	head=JournalAddHeader(buffer);

	JournalLock(journal);

	BufferLock(buffer);

repeat:
	if (head->transaction && head->transaction != trans)
	{
		if (head->list == JOURN_SHADOW)
		{
			JournalUnlock(journal);
			KePrint("shadow wait, %#X, %u\n", head, JournHeadToBuffer(head)->blockNum);
			WAIT_ON(&head->wait, head->list != JOURN_SHADOW);
			JournalLock(journal);
			goto repeat;
		}
		
		if (head->list != JOURN_FORGET)
		{
		}
		
		head->nextTransaction=trans;
	}

	if (!head->transaction)
	{
		head->transaction=trans;
		JournalFileBuffer(head, trans, JOURN_RESERVED);
	}
	
	BufferUnlock(buffer);
	
	if (handle->numBlocks <= 0)
	{
		KePrint("numBlocks <= 0\n");
		cli(); hlt();
		return 0;
	}
	
	handle->numBlocks--;
	
//	KePrint("blocks left: %u\n", handle->numBlocks);

	JournalUnlock(journal);

	return 0;
}

SYMBOL_EXPORT(JournalGetWriteAccess);

int JournalDirtyMetadata(struct JournalHandle* handle, struct Buffer* buffer)
{
	struct JournalTrans* trans=handle->transaction;
	struct Journal* journal=trans->journal;
	struct JournalHead* head=BufferToJournHead(buffer);

	JournalLock(journal);

	JournalFileBuffer(head, trans, JOURN_METADATA);

	JournalUnlock(journal);

	return 0;
}

SYMBOL_EXPORT(JournalDirtyMetadata);

int JournalForceCommit(struct Journal* journal)
{
	struct JournalHandle* handle;

	handle=JournalStart(journal, 1);

	handle->sync=1;

	/* The sync forces the commit to disk. */
	JournalStop(handle);

	return 0;
}

SYMBOL_EXPORT(JournalForceCommit);
