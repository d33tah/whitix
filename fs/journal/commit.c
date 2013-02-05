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
#include <fs/bcache.h>
#include <fs/journal.h>
#include <llist.h>
#include <module.h>
#include <print.h>
#include <sched.h>
#include <wait.h>

int JournalStartCommit(struct Journal* journal, struct JournalTrans* trans)
{
	/* Do checks. */
	journal->commitRequest=trans->transId;

	WakeUp(&journal->commitWait);
	return 0;
}

SYMBOL_EXPORT(JournalStartCommit);

int JournalWaitCommit(struct Journal* journal, DWORD id)
{
	while (id > journal->commitSequence)
	{
		WakeUp(&journal->commitWait);
		SLEEP_ON(&journal->commitWait);
	}

	return 0;
}

SYMBOL_EXPORT(JournalWaitCommit);

struct JournalHead* JournalCommitMetadata(struct Journal* journal, struct JournalTrans* commitTrans)
{
	struct JournalHead* curr, *curr2, *newHead;
	struct JournalHead* descriptor=NULL; /* On-disk structure. */
	struct JournalHeader* dHeader;
	struct Buffer* buffers[64];
	int bufIndex=0;
	int firstTag=0;
	struct JournalBlockTag* tag;
	BYTE* tagPointer=NULL;
	DWORD spaceLeft=0;
	DWORD blockNo;

	ListForEachEntrySafe(curr, curr2, &commitTrans->metaDataList, next)
	{
		int tagFlags=0;

		if (!descriptor)
		{
			struct Buffer* buffer;

			descriptor=JournalGetDescriptorBuffer(journal);
			buffer=JournHeadToBuffer(descriptor);

			dHeader=(struct JournalHeader*)(buffer->data);

			dHeader->magic=CpuToBe32(JFS_MAGIC);
			dHeader->blockType=CpuToBe32(JFS_DESC_BLOCK);
			dHeader->sequence=CpuToBe32(commitTrans->transId);

			tagPointer=&buffer->data[sizeof(struct JournalHeader)];	
			spaceLeft=journal->sectorSize-sizeof(struct JournalHeader);
			firstTag=1;

			buffers[bufIndex++]=buffer;

			JournalFileBuffer(descriptor, commitTrans, JOURN_LOGCTL);
		}

		JournalNextLogBlock(journal, &blockNo);

		JournalWriteMetadataBuffer(commitTrans, curr, &newHead, blockNo);

		/* We get a copy of curr, newHead, that points at the journal block
		 * to write to. The real write of metadata to the disk takes place
		 * in the checkpoint.
		 */

		if (!firstTag)
			tagFlags |= JOURN_FLAG_SAME_UUID;

		tag=(struct JournalBlockTag*)tagPointer;
		tag->blockNum=CpuToBe32(JournHeadToBuffer(curr)->blockNum);
		tag->flags=CpuToBe32(tagFlags);

		tagPointer+=sizeof(struct JournalBlockTag);
		spaceLeft-=sizeof(struct JournalBlockTag);

		if (firstTag)
		{
			memcpy(tagPointer, journal->uuid, 16);
			tagPointer+=16;
			spaceLeft-=16;
			firstTag=0;
		}
		
		buffers[bufIndex++]=JournHeadToBuffer(newHead);
		
		/* Are we at the end of the list, or has the descriptor block run out of
		 * space? 
		 */
		if (bufIndex == 64 || curr2->next.next == &commitTrans->metaDataList ||
		 	spaceLeft < sizeof(struct JournalBlockTag)+16)
		{
			int i;
			
//			KePrint("Submitting %d buffers\n", bufIndex);
			tag->flags|=CpuToBe32(JOURN_FLAG_LAST_TAG);
			
			JournalUnlock(journal);
			
			for (i=0; i<bufIndex; i++)
			{
				BlockWrite(buffers[i]->device, buffers[i]);
			}
			
			JournalLock(journal);
		}
		
	}

	return descriptor;
}

int JournalWaitForIO(struct Journal* journal, struct JournalTrans* commitTrans)
{
	struct JournalHead* curr, *curr2;
	struct JournalHead* shadow=ListEntry(commitTrans->shadowList.next, struct JournalHead, next), *shadow2;
	
	/* Go through the journal I/O list. */
	ListForEachEntrySafe(curr, curr2, &commitTrans->ioList, next)
	{
		struct Buffer* buffer;
		
		/* Wait for the I/O to complete and free the header. */
		buffer=JournHeadToBuffer(curr);
		WaitForBuffer(buffer);
		JournalUnfileBuffer(curr);
		JournalRemoveHeader(curr);
		
		/* Get the corresponding shadow buffer, and file that to a checkpoint list. */
		shadow2=ListEntry(shadow->next.next, struct JournalHead, next);
			
//		KePrint("Filing %u\n", JournHeadToBuffer(shadow)->blockNum);
		JournalFileBuffer(shadow, commitTrans, JOURN_FORGET);
		WakeUp(&shadow->wait);
		shadow=shadow2;
	}
	
	return 0;
}

int JournalWriteCommitRecord(struct Journal* journal, struct JournalTrans* commitTrans)
{
	struct JournalHead* head;
	struct Buffer* buffer;
	DWORD i;
	
	head=JournalGetDescriptorBuffer(journal);
	buffer=JournHeadToBuffer(head);
	
	for (i=0; i<journal->sectorSize; i+=512)
	{
		struct JournalHeader* header=(struct JournalHeader*)(buffer->data+i);
		header->magic=CpuToBe32(JFS_MAGIC);
		header->blockType=CpuToBe32(JFS_COMMIT_BLOCK);
		header->sequence=CpuToBe32(commitTrans->transId);
	}
	
	JournalUnlock(journal);
	
	BlockWrite(buffer->device, buffer);
	WaitForBuffer(buffer);
	BufferRelease(buffer);
	
	return 0;
}

int JournalCommitTransaction(struct Journal* journal)
{
	struct JournalTrans* commitTrans;

	JournalLock(journal); /* The journal is unlocked in WriteCommitRecord. */

	commitTrans=journal->currTransaction;

	KePrint(KERN_DEBUG "Committing %d, %d\n", commitTrans->transId, commitTrans->updates);
	
	commitTrans->state=JTRANS_LOCKED;

	while (commitTrans->updates)
	{
		JournalUnlock(journal);
		KePrint("wait: %u ", commitTrans->updates);
		WAIT_ON(&journal->waitUpdates, !commitTrans->updates);
		JournalLock(journal);
	}

	/* Revoke! */

	journal->commitSequence=commitTrans->transId;
	journal->committingTrans=commitTrans;
	journal->currTransaction=NULL;
	
	commitTrans->logStart=journal->head;

	/* Phase 2 */

	/* Data! */

	/* Write out metadata to the journal. */
	JournalCommitMetadata(journal, commitTrans);
	
	/* Make sure the data is written to disk, and the metadata is written to the
	 * journal.
	 */
	JournalWaitForIO(journal, commitTrans);
	
	/* Make sure the descriptor records are written to disk */
	ListAddTail(&commitTrans->next, &journal->checkpointTrans);
	
	/* Write the commit record. */
	JournalWriteCommitRecord(journal, commitTrans);

	/* TEST: Checkpoint the transaction. */
	JournalCheckpoint(journal);

	WakeUp(&journal->commitWaitDone);

	return 0;
}

SYMBOL_EXPORT(JournalCommitTransaction);
