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

#include <byteswap.h>
#include <fs/journal.h>
#include <fs/vfs.h>
#include <malloc.h>
#include <module.h>
#include <print.h>
#include <slab.h>
#include <sched.h>

extern struct Cache* journalCache;

static struct Journal* JournalAllocate()
{
	return MemCacheAlloc(journalCache);
}

int JournalBlockMap(struct Journal* journal, DWORD blockNo, DWORD* ret)
{
//	int err;
	struct VNode* vNode=journal->vNode;

	*ret=vNode->vNodeOps->blockMap(vNode, blockNo, 0);

	/* TODO: Handle case where we could not find the start. */

	return 0;
}

struct JournalHead* JournalAllocHeader()
{
	return (struct JournalHead*)MemAlloc(sizeof(struct JournalHead));
}

struct JournalHead* JournalAddHeader(struct Buffer* buffer)
{
	struct JournalHead* ret;

	PreemptDisable();

	if (BufferJournal(buffer))
	{
		ret=BufferToJournHead(buffer);
	}else{
		ret=JournalAllocHeader();

		buffer->flags |= 1 << BUFFER_JOURNAL;
		buffer->privData=ret;
		ret->buffer=buffer;
		INIT_WAITQUEUE_HEAD(&ret->wait);
		INIT_LIST_HEAD(&ret->next);
		
		BufferGet(buffer);
	}
	
	ret->refs++;
	
	PreemptEnable();
	
//	KePrint("JournalAddHeader = %#X, %u\n", ret, ret->refs);

	return ret;
}

void JournalRemoveHeader(struct JournalHead* header)
{
	header->refs--;
	
	if (!header->refs)
	{
		PreemptDisable();
		
		BufferRelease(header->buffer);
		header->buffer->privData=NULL;
		header->buffer->flags &= ~(1 << BUFFER_JOURNAL);
		WakeUp(&header->wait);
		
		MemFree(header);
		PreemptEnable();
	}
}

void JournalUpdateSuperBlock(struct Journal* journal)
{
	DWORD blockNo;
	struct Buffer* buffer;
	struct JournalSuperBlock* jSb;
	
	JournalBlockMap(journal, 0, &blockNo);
	
	buffer=JournalBlockRead(journal, blockNo);
	
	jSb=(struct JournalSuperBlock*)(buffer->data);
	
	jSb->sequence=CpuToBe32(journal->tailSequence);
	jSb->start=CpuToBe32(journal->tail);
	
	BlockWrite(buffer->device, buffer);
	
	WaitForBuffer(buffer);
}

extern void SleepWakeup(void* data);

void JournalCommitSetTimer(struct Timer* commitTimer, struct Journal* journal)
{
	/* Set up timer. */
	commitTimer->func=SleepWakeup;
	commitTimer->expires=journal->commitInterval*100000;
	commitTimer->data=currThread;

	TimerAdd(commitTimer);
}

/* Journal commit thread. */

/* TODO: Move to commit.c */
void JournalCommitter(void* arg)
{
	struct Journal* journal=(struct Journal*)arg;
	struct Timer commitTimer;

	INIT_WAITQUEUE_HEAD(&journal->commitWait);
	INIT_WAITQUEUE_HEAD(&journal->commitWaitDone);

	KePrint(KERN_INFO "JournalCommitter: started journal with commit interval of %d"
		" seconds\n", journal->commitInterval);

	while (1)
	{
		if (journal->flags & JOURN_UNMOUNT)
			break;
			
		/* There should be a transaction to commit. */
		if (journal->commitSequence != journal->commitRequest)
		{
			TimerRemove(&commitTimer);
			JournalCommitTransaction(journal);
		}

		/* May not always want timer running, esp. if we are waiting for commits. */
		JournalCommitSetTimer(&commitTimer, journal);

		SLEEP_ON(&journal->commitWait);
		
		TimerRemove(&commitTimer);

		/* Get the running transaction, if we timed out, and commit that. Check expires. */
		if (journal->currTransaction && currTime.seconds >= journal->currTransaction->expires)
			journal->commitRequest=journal->currTransaction->transId;
	}
	
	TimerRemove(&commitTimer);
	
	journal->flags=0;
	WakeUp(&journal->commitWaitDone);
	
	ThrDoExitThread();
}

int JournalWriteMetadataBuffer(struct JournalTrans* commitTrans, struct JournalHead* old, 
	struct JournalHead** new, DWORD blockNo)
{
	struct Buffer* newBuf, *oldBuf;

	oldBuf=JournHeadToBuffer(old);
	newBuf=BlockBufferAlloc(oldBuf->device, blockNo);

	/* Buffer is now locked, so copy over the data from old's buffer. */
	memcpy(newBuf->data, oldBuf->data, oldBuf->device->softBlockSize);

	*new=JournalAddHeader(newBuf);
	(*new)->transaction=NULL;
	
	BufferUnlock(newBuf);

	JournalFileBuffer(old, commitTrans, JOURN_SHADOW);
	JournalFileBuffer(*new, commitTrans, JOURN_IO);

	return 0;
}

int JournalNextLogBlock(struct Journal* journal, DWORD* ret)
{
	DWORD blockNum;

	blockNum=journal->head;

	journal->head++;
	journal->free--;
	
	/* Check for wraparound! */

	return JournalBlockMap(journal, blockNum, ret);
}

struct JournalHead* JournalGetDescriptorBuffer(struct Journal* journal)
{
	int err;
	DWORD blockNum;
	struct Buffer* buffer;

	err=JournalNextLogBlock(journal, &blockNum);

	if (err)
		return NULL;

	buffer=JournalBlockRead(journal, blockNum);

	if (!buffer)
		return NULL;

	memset(buffer->data, journal->sectorSize, 0);

	return JournalAddHeader(buffer);
}

int JournalDestroy(struct Journal* journal)
{
	/* Shut down the committer thread. */
	journal->flags|=JOURN_UNMOUNT;
	WakeUp(&journal->commitWait);
	WAIT_ON(&journal->commitWaitDone, !journal->flags);
	
	if (journal->currTransaction)
		JournalCommitTransaction(journal);
		
	JournalLock(journal);
	
	while (!ListEmpty(&journal->checkpointTrans))
		JournalCheckpoint(journal);
	
	journal->tail=0;
	journal->tailSequence=++journal->transSequence;
	
	JournalUpdateSuperBlock(journal);
	
	VNodeRelease(journal->vNode);
	
	JournalUnlock(journal);	
	MemCacheFree(journalCache, journal);
	
	return 0;	
}

SYMBOL_EXPORT(JournalDestroy);

void JournalSetUuid(struct Journal* journal, BYTE* uuid)
{
	memcpy(journal->uuid, uuid, 16);
}

SYMBOL_EXPORT(JournalSetUuid);

struct Journal* JournalCreate(struct VNode* vNode)
{
	struct Journal* journal;
	DWORD blockNo;
	struct Buffer* headerBuf;
	struct JournalSuperBlock* jSb;
	struct VfsSuperBlock* superBlock=vNode->superBlock;

	journal=JournalAllocate();

	if (!journal)
		return NULL;

	journal->vNode=vNode;
	ListRemove(&vNode->next); /* So that SysUnmount can unmount. Is this the right way to do it? */

	/* Read the journal superblock. */
	JournalBlockMap(journal, 0, &blockNo);
	
	if ((int)blockNo == -1)
	{
		KePrint(KERN_ERROR "JournalCreate: could not read the journal superblock\n");
		MemCacheFree(journalCache, journal);
		return NULL;
	}

	headerBuf=JournalBlockRead(journal, blockNo);

	jSb=(struct JournalSuperBlock*)(headerBuf->data);

	if (BeToCpu32(jSb->header.magic) != JFS_MAGIC ||
		BeToCpu32(jSb->blockSize) != BYTES_PER_SECTOR(superBlock))
	{
		KePrint(KERN_ERROR "JournalCreate: invalid superblock.\n");
		/* TODO: Abort. */
	}

	switch (BeToCpu32(jSb->header.blockType))
	{
		case JFS_SUPERBLOCK_V1:
			journal->version=1;
			break;

		case JFS_SUPERBLOCK_V2:
			journal->version=2;
			break;

		default:
			/* Error. */
			KePrint(KERN_ERROR "JournalCreate: unrecognised journal version.\n");
	}

	journal->maxLength=vNode->size/BYTES_PER_SECTOR(vNode->superBlock);
	journal->sectorSize=BYTES_PER_SECTOR(vNode->superBlock);
	journal->transSequence=1; /* Check */

	if (BeToCpu32(jSb->maxLen) < journal->maxLength)
		journal->maxLength=BeToCpu32(jSb->maxLen);
	else if (BeToCpu32(jSb->maxLen) > journal->maxLength)
		KePrint(KERN_ERROR "JournalCreate: journal file too short.\n");

	journal->maxTransactionBuffers=journal->maxLength/4;

	KePrint(KERN_DEBUG "JournalCreate: found journal, version %d, length = %#X\n",
		journal->version, journal->maxLength);

	/* Start the per-journal commit kernel thread. */
	journal->commitThread=ThrCreateKernelThreadArg(JournalCommitter, journal);
	journal->commitInterval=JOURN_COMMIT_INTERVAL;

	JournalRecover(journal, jSb);

	/* Set up transaction sequence numbers. */
	journal->commitSequence=journal->transSequence-1;
	journal->commitRequest=journal->commitSequence;
	journal->tailSequence=journal->transSequence;

	journal->head=journal->tail=BeToCpu32(jSb->first);
	journal->free=BeToCpu32(jSb->maxLen)-journal->head;

	INIT_LIST_HEAD(&journal->checkpointTrans);
	INIT_WAITQUEUE_HEAD(&journal->waitUpdates);

	ThrStartThread(journal->commitThread);

	/* TODO: Wait for it to get started up before continuing */

	return journal;
}

SYMBOL_EXPORT(JournalCreate);
