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
#include <module.h>
#include <print.h>
#include <sched.h>
#include <slab.h>
#include <task.h>

struct Thread* journalCommitter=NULL;

struct Cache* journalCache=NULL, *journHandleCache=NULL, *journTransCache=NULL;

int JournalInit()
{
	/* Init caches. */
	journalCache=MemCacheCreate("Journal cache", sizeof(struct Journal), NULL, NULL, 0);
	journHandleCache=MemCacheCreate("Journal handle cache", 
		sizeof(struct JournalHandle), NULL, NULL, 0);
	journTransCache=MemCacheCreate("Journal transaction cache",
		sizeof(struct JournalTrans), NULL, NULL, 0);

	return 0;
}

ModuleInit(JournalInit);
