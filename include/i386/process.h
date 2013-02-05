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

#ifndef PROCESS_H
#define PROCESS_H

#include <task.h>
#include <typedefs.h>

#define TSS_ADDR	0xC0002100

struct TSS
{
	DWORD backlink,esp0,ss0,esp1,ss1,esp2,ss2,cr3,eip,eflags,eax,ecx,
		edx,ebx,esp,ebp,esi,edi,es,cs,ss,ds,fs,gs,ldt PACKED;
	WORD trace,bitmap PACKED;
}PACKED;

struct ArchThreadData
{
	DWORD esp0;
};

int ThrArchInit();
struct Thread* ThrArchSwitch(struct Thread* prev,struct Thread* next);
int ThrArchCreateThread(struct Thread* thread,DWORD entry,int user,DWORD stackP, void* argument);
void ThrArchPrintContext(struct Context* curr);
int ThrArchDestroyThread(struct Thread* thread);
void i386ContextSwitch(struct Thread* prev,struct Thread* next);

#endif
