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

#ifndef LLIST_H
#define LLIST_H

#include <typedefs.h>

struct ListHead
{
	struct ListHead* next, *prev;
};

/***********************************************************************
 *
 * MACRO:		LIST_HEAD_INIT
 *
 * DESCRIPTION: Construct a list head at compile time. Not used generally,
 *				except in LIST_HEAD.
 *
 * PARAMETERS:	name - name of the list head variable.
 *
 * EXAMPLE:		struct ListHead head = LIST_HEAD_INIT(head)
 *
 ***********************************************************************/

#define LIST_HEAD_INIT(name) { &(name), &(name) }

/***********************************************************************
 *
 * MACRO:		LIST_HEAD_INIT
 *
 * DESCRIPTION: Construct a list head at runtime.
 *
 * PARAMETERS:	ptr - address (typically) of the list head.
 *
 * EXAMPLE:		INIT_LIST_HEAD(&head)
 *
 ***********************************************************************/

#define INIT_LIST_HEAD(ptr) do { (ptr)->next=(ptr); (ptr)->prev=(ptr); } while(0)

/***********************************************************************
 *
 * MACRO:		LIST_HEAD
 *
 * DESCRIPTION: Construct a list head at compile time.
 *
 * PARAMETERS:	name - name of the list head variable to construct
 *
 * EXAMPLE:		LIST_HEAD(threadList)
 *
 ***********************************************************************/

#define LIST_HEAD(name) struct ListHead name=LIST_HEAD_INIT(name)

static inline void DoListAdd(struct ListHead* new,struct ListHead* prev,struct ListHead* next)
{
	next->prev=new;
	new->next=next;
	new->prev=prev;
	prev->next=new;
}

static inline void ListAdd(struct ListHead* new,struct ListHead* head)
{
	DoListAdd(new,head,head->next);
}

/* Adds ListHead to the prev of head, which is the end of the list */

static inline void ListAddTail(struct ListHead* new,struct ListHead* head)
{
	DoListAdd(new,head->prev,head);
}

static inline void DoListRemove(struct ListHead* prev,struct ListHead* next)
{
	next->prev=prev;
	prev->next=next;
}

static inline void ListRemove(struct ListHead* entry)
{
	DoListRemove(entry->prev,entry->next);
}

static inline void ListRemoveInit(struct ListHead* entry)
{
	DoListRemove(entry->prev,entry->next);
	INIT_LIST_HEAD(entry);
}

static inline int ListEmpty(struct ListHead* head)
{
	return (head->next == head);
}

static inline void DoListSpliceTail(struct ListHead* list, struct ListHead* head)
{
	struct ListHead* first = list->next;
	struct ListHead* last = list->prev;
	struct ListHead* at = head->prev;

	first->prev = at;
	at->next = first;

	last->next = head;
	head->prev = last;
}

static inline void ListSpliceTail(struct ListHead* list, struct ListHead* head)
{
	if (!ListEmpty(list))
		DoListSpliceTail(list, head);	
}

static inline void ListSpliceTailInit(struct ListHead* list, struct ListHead* head)
{
	if (!ListEmpty(list))
	{
		DoListSpliceTail(list, head);
		INIT_LIST_HEAD(list);
	}
}

/* Minus the address of the list_struct within the struct by the start of the struct */
#define ListEntry(ptr,type,member) \
	((type*)(((char*)ptr) - OffsetOf(type,member)))

#define ListForEachEntry(pos,head,member) \
	for (pos=ListEntry((head)->next,typeof(*pos),member); \
	 &(pos->member) != head; \
	 pos=ListEntry(pos->member.next, typeof(*pos), member))

#define ListForEachEntrySafe(pos,n,head,member) \
	for (pos=ListEntry((head)->next,typeof(*pos),member), \
	n=ListEntry(pos->member.next,typeof(*pos),member); \
	&pos->member!=(head); \
	pos=n,n=ListEntry(n->member.next,typeof(*n),member))

#define ListForEach(pos,head) \
	for (pos=head; pos != head; pos=pos->next)

#endif
