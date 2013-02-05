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

#include <fs/bcache.h>
#include <fs/journal.h>
#include <fs/vfs.h>
#include <byteswap.h>
#include <module.h>
#include <print.h>
#include <wait.h>

#define JournalWrap(journal, pos) \
	if ((pos) >= (journal)->maxLength) \
		(pos) -= ((journal)->maxLength - (journal)->head)

int JournalCountTags(struct Buffer* buffer, DWORD size)
{
	BYTE* tagPointer;
	struct JournalBlockTag* tag;
	int num=0;
	
	tagPointer=&buffer->data[sizeof(struct JournalHeader)];
	
	while (((DWORD)tagPointer-(DWORD)buffer->data+sizeof(struct JournalBlockTag)) <= size)
	{
		tag=(struct JournalBlockTag*)tagPointer;
		num++;
		tagPointer+=sizeof(struct JournalBlockTag);
		
		if (!(tag->flags & BeToCpu32(JOURN_FLAG_SAME_UUID)))
			tagPointer+=16;
			
		if (tag->flags & BeToCpu32(JOURN_FLAG_LAST_TAG))
			break;
	}
	
	return num;
}

int JournalReplayBlocks(struct Journal* journal, struct Buffer* buffer, DWORD* logBlock)
{
	BYTE* tagPointer;
	struct JournalBlockTag* tag;
	struct Buffer* journEntry, *onDisk;
	DWORD ioBlock, blockNo;
	
	tagPointer=&buffer->data[sizeof(struct JournalHeader)];
	
	while (((DWORD)tagPointer-(DWORD)buffer->data+sizeof(struct JournalBlockTag)) <= journal->sectorSize)
	{
		DWORD flags;
		
		tag=(struct JournalBlockTag*)tagPointer;
		flags=BeToCpu32(tag->flags);
		
		ioBlock=*logBlock;
		*logBlock=(*logBlock)+1;
		JournalWrap(journal, *logBlock);
		
		/* Read the block of data in the journal */
		JournalBlockMap(journal, ioBlock, &blockNo);
		
		journEntry=JournalBlockRead(journal, blockNo);
		
		/* Handle error. */
		blockNo=BeToCpu32(tag->blockNum);
		
		onDisk=BlockBufferAlloc(journEntry->device, blockNo);
		memcpy(onDisk->data, journEntry->data, journal->sectorSize);
		
		BlockWrite(onDisk->device, onDisk);
		
		BufferUnlock(onDisk);
		WaitForBuffer(onDisk);
		BufferRelease(onDisk);
		
		BufferRelease(journEntry);
		
		tagPointer+=sizeof(struct JournalBlockTag);
		
		if (!(tag->flags & BeToCpu32(JOURN_FLAG_SAME_UUID)))
			tagPointer+=16;
			
		if (tag->flags & BeToCpu32(JOURN_FLAG_LAST_TAG))
			break;
		
	}
	
	return 0;
}

int JournalDoOnePass(struct Journal* journal, struct JournalSuperBlock* jSb, 
	struct JournalRecovery* info, int type)
{
	DWORD firstCommitId, nextCommitId;
	DWORD nextLogBlock, blockNo;
	DWORD blockType, sequence;
	struct Buffer* buffer;
	struct JournalHeader* head;
	
	nextCommitId=BeToCpu32(jSb->sequence);
	nextLogBlock=BeToCpu32(jSb->start);
	
	firstCommitId=nextCommitId;
	
	if (type == JOURN_PASS_SCAN)
		info->firstTransaction=firstCommitId;
	
	while (1)
	{
		KePrint("Scanning for %u, %u/%u\n", nextCommitId, nextLogBlock, journal->maxLength);
		
		JournalBlockMap(journal, nextLogBlock, &blockNo);
		
		buffer=JournalBlockRead(journal, blockNo);
		
		nextLogBlock++;
		JournalWrap(journal, nextLogBlock);
		
		head=(struct JournalHeader*)(buffer->data);
		
		if (head->magic != CpuToBe32(JFS_MAGIC))
		{
			BufferRelease(buffer);
			break;
		}
		
		sequence=BeToCpu32(head->sequence);
		blockType=BeToCpu32(head->blockType);
		
		if (sequence != nextCommitId)
		{
			BufferRelease(buffer);
			break;
		}
		
		switch (blockType)
		{
			case JFS_DESC_BLOCK:
				if (type != JOURN_PASS_REPLAY)
				{
					nextLogBlock+=JournalCountTags(buffer, journal->sectorSize);
					/* WRAP */
					BufferRelease(buffer);
					continue;
				}
				
				JournalReplayBlocks(journal, buffer, &nextLogBlock);
				
				BufferRelease(buffer);
				continue;
				
			case JFS_COMMIT_BLOCK:
				BufferRelease(buffer);
				nextCommitId++;
				continue;
				
			default:
				KePrint("blockType = %u\n", blockType);
		}
	}
	
	info->endTransaction=nextCommitId;
	
	return 0;
}

int JournalRecover(struct Journal* journal, struct JournalSuperBlock* jSb)
{
	DWORD start=BeToCpu32(jSb->start);
	struct JournalRecovery recovery;

	if (!start)
	{
		KePrint(KERN_DEBUG "JournalRecover: no recovery needed. Last transaction %d\n",
			BeToCpu32(jSb->sequence));

		journal->transSequence=BeToCpu32(jSb->sequence)+1;
		return 0;
	}

	JournalDoOnePass(journal, jSb, &recovery, JOURN_PASS_SCAN);
	
	JournalDoOnePass(journal, jSb, &recovery, JOURN_PASS_REPLAY);
	
	journal->transSequence=recovery.endTransaction++;
	
	/* Sync all buffers. */
	
	KePrint("Finished recovery\n");
	
	return 0;
}
