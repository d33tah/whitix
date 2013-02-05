/******************************************************************************
 *
 * Name: journal.h - defines, structures and function prototypes for journals.
 *
 ******************************************************************************/

/* 
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

#ifndef VFS_JOURNAL_H
#define VFS_JOURNAL_H

#include <preempt.h>
#include <task.h>
#include <sched.h>
#include <timer.h>
#include <typedefs.h>
#include <wait.h>

/* In-memory structures. */

/* Types of list the head can be added to. */
#define JOURN_RESERVED	1
#define JOURN_METADATA	2
#define JOURN_LOGCTL	3
#define JOURN_SHADOW	4
#define JOURN_IO		5
#define JOURN_FORGET	6

struct JournalHead
{
	struct Buffer* buffer;
	struct JournalTrans* transaction, *nextTransaction;
	int list, refs;
	struct ListHead next;
	WaitQueue wait;
};

#define JTRANS_RUNNING	1
#define JTRANS_LOCKED	2

struct JournalTrans
{
	struct Journal* journal;
	int state, transId, outstandingBlocks;
	DWORD expires, logStart;
	int updates, handleCount;

	/* Lists */
	struct ListHead reservedList;
	struct ListHead metaDataList;
	struct ListHead logControlList;
	struct ListHead shadowList;
	struct ListHead ioList;
	struct ListHead forgetList;
	
	struct ListHead next;
};

/* JournalHandle denotes one atomic update to the filesystem. */
struct JournalHandle
{
	int refs, numBlocks, sync;
	struct JournalTrans* transaction;
};

#define JOURN_COMMIT_INTERVAL	2 /* seconds */

#define JOURN_UNMOUNT	1

struct Journal
{
	struct VNode* vNode;
	int version;
	DWORD maxLength, sectorSize, maxTransactionBuffers;
	BYTE uuid[16];

	/* Sequence numbers. */
	DWORD transSequence, commitSequence, tailSequence, commitRequest;

	/* Block management. */
	DWORD head, tail, free;
	
	/* Checkpoints. */
	struct ListHead checkpointTrans;

	struct JournalTrans* currTransaction, *committingTrans;
	struct Thread* commitThread;
	WaitQueue commitWait, commitWaitDone, waitUpdates;
	int commitInterval, flags;
};

#define JOURN_PASS_SCAN		0
#define JOURN_PASS_REVOKE	1
#define JOURN_PASS_REPLAY	2

struct JournalRecovery
{
	DWORD firstTransaction, endTransaction;
};

#define JFS_MAGIC	0xC03B3998

struct JournalHeader
{
	DWORD magic;
	DWORD blockType;
	DWORD sequence;
}PACKED;

#define JOURN_FLAG_SAME_UUID	2
#define JOURN_FLAG_LAST_TAG		8

struct JournalBlockTag
{
	DWORD blockNum;
	DWORD flags;
};

/* Block types. */
#define JFS_DESC_BLOCK		1
#define JFS_COMMIT_BLOCK	2
#define JFS_SUPERBLOCK_V1	3
#define JFS_SUPERBLOCK_V2	4

/* On-disk structure for the jbd superblock. All fields are in big endian order. */
struct JournalSuperBlock
{
	struct JournalHeader header;

	DWORD blockSize;
	DWORD maxLen;
	DWORD first;

	DWORD sequence;
	DWORD start;

	DWORD errNo;
};

/* Prototypes */

/* journal.c */
struct Journal* JournalCreate(struct VNode* vNode);
int JournalDestroy(struct Journal* journal);
struct JournalHandle* JournalStart(struct Journal* journal, int numBlocks);
int JournalStop(struct JournalHandle* handle);
struct JournalHead* JournalAddHeader(struct Buffer* buffer);
struct JournalHead* JournalGetDescriptorBuffer(struct Journal* journal);
void JournalUpdateSuperBlock(struct Journal* journal);
int JournalBlockMap(struct Journal* journal, DWORD blockNo, DWORD* ret);
void JournalSetUuid(struct Journal* journal, BYTE* uuid);
int JournalNextLogBlock(struct Journal* journal, DWORD* ret);
int JournalWriteMetadataBuffer(struct JournalTrans* commitTrans, struct JournalHead* old, 
	struct JournalHead** new, DWORD blockNo);
void JournalRemoveHeader(struct JournalHead* header);

/* transaction.c */
int JournalGetWriteAccess(struct JournalHandle* handle, struct Buffer* buffer);
int JournalDirtyMetadata(struct JournalHandle* handle, struct Buffer* buffer);
int JournalForceCommit(struct Journal* journal);
void JournalFileBuffer(struct JournalHead* head, struct JournalTrans* trans, int type);
void JournalUnfileBuffer(struct JournalHead* head);

/* commit.c */
int JournalStartCommit(struct Journal* journal, struct JournalTrans* trans);
int JournalWaitCommit(struct Journal* journal, DWORD id);
int JournalCommitTransaction(struct Journal* journal);

/* recovery.c */
int JournalRecover(struct Journal* journal, struct JournalSuperBlock* jSb);

/* checkpoint.c */
int JournalCheckpoint(struct Journal* journal);

/* Defines */

#define JournalBlockRead(journal, blockNum) \
	BlockRead(journal->vNode->superBlock->sDevice, blockNum)

#define JournalCurrHandle() \
	(currThread)->currHandle

#define JournalLock(journal) \
	PreemptDisable(); \
	(void)journal

#define JournalUnlock(journal) \
	PreemptEnable()

#define JournHeadToBuffer(head) \
	(head)->buffer

#define BufferToJournHead(buffer) \
	(buffer)->privData

#endif
