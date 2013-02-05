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

int JournalCleanupTail(struct Journal* journal)
{
	/* Get the first entry on the checkpoint transaction list. */
	struct JournalTrans* transaction;
	DWORD firstTid, blockNo;
	int freed;
	
	if (!ListEmpty(&journal->checkpointTrans))
	{
		transaction=ListEntry(journal->checkpointTrans.next, struct JournalTrans, next);

		firstTid=transaction->transId;
		blockNo=transaction->logStart;
		
//		KePrint("transaction->id = %u, %u\n", transaction->transId, blockNo);
	}else if ((transaction = journal->committingTrans) || (transaction = journal->currTransaction))
	{
		firstTid=transaction->transId;
		blockNo=transaction->logStart;
	}else{
//		KePrint("JournalCleanupTail: list empty!\n");
		
		firstTid=journal->transSequence;
		blockNo=journal->head;
	}
	
	if (journal->tailSequence == firstTid)
		return 1;
	
	freed=blockNo-journal->tail;
	
	KePrint("Journal tail from %d to %d (offset %u), freed %d\n", journal->tailSequence, firstTid, blockNo, freed);
	
	journal->free+=freed;
	journal->tailSequence=firstTid;
	journal->tail=blockNo;
	
	return 0;
}

int JournalCheckpoint(struct Journal* journal)
{
	struct JournalTrans* curr, *curr2;
	struct JournalHead* currHead, *currHead2;
	int ret;

	JournalLock(journal);
	
	ret=JournalCleanupTail(journal);
	
	if (ret <= 0)
		goto out;
	
	ListForEachEntry(curr, &journal->checkpointTrans, next)
	{
		ListForEachEntry(currHead, &curr->forgetList, next)
		{
			BlockWrite(currHead->buffer->device, currHead->buffer);
//			KePrint("Wrote %u\n", currHead->buffer->blockNum);
		}
	}
	
	ListForEachEntrySafe(curr, curr2, &journal->checkpointTrans, next)
	{
		ListForEachEntrySafe(currHead, currHead2, &curr->forgetList, next)
		{
			WaitForBuffer(currHead->buffer);
			ListRemove(&currHead->next);
			JournalRemoveHeader(currHead);
		}
		
		INIT_LIST_HEAD(&curr->forgetList);
		
		ListRemove(&curr->next);
	}
		
	INIT_LIST_HEAD(&journal->checkpointTrans);
	
	/* Clean up the tail. */
	JournalCleanupTail(journal);
	
	/* Update the superblock. */
	JournalUpdateSuperBlock(journal);

out:
	JournalUnlock(journal);
	
	return 0;
}

SYMBOL_EXPORT(JournalCheckpoint);
