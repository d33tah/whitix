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

#include <i386/physical.h>
#include <pg_alloc.h>
#include <print.h>
#include <sections.h>
#include <i386/multiboot.h>
#include <addresses.h>

#define PFN_MAX		((0x100000000ULL) >> PAGE_SHIFT)

/*** GLOBALS ***/

extern struct MbMemoryMap* memoryMap;
initdata int numEntries = 0;

initdata struct E820Entry entries[32]; /* All three methods copy data into this entry array ... */
initdata int entryCount=0; 	      /* ... and set this count variable */

/* ChangeMember - used when sorting the entries in ReadE820 */
struct ChangeMember
{
	struct MbMemoryMap* pBios;
	QWORD addr;
};

/***********************************************************************
 *
 * FUNCTION:	SetupE820Sort
 *
 * DESCRIPTION: Set up the change list for sorting the map.
 *
 * PARAMETERS:	changePoint - list of change points
 *		changePointList - pointers to changePoints
 *		memEntries - list of E820 entries (from memory)
 *		numEntries - number of E820 entries
 *
 * RETURNS:	None.
 *
 ***********************************************************************/

initcode int SetupE820Sort(struct ChangeMember* changePoint[],struct ChangeMember* changePointList,struct MbMemoryMap* memEntries,int numEntries)
{
	int changeIndex=0;
	int i;

	for (i=0; i<2*numEntries; i++)
		changePoint[i]=&changePointList[i];

	for (i=0; i<numEntries; i++)
	{
		if (memEntries[i].length)
		{
			changePoint[changeIndex]->addr=memEntries[i].baseAddr;
			changePoint[changeIndex++]->pBios=&memEntries[i];
			changePoint[changeIndex]->addr=memEntries[i].baseAddr+memEntries[i].length;
			changePoint[changeIndex++]->pBios=&memEntries[i];
		}
	}

	return changeIndex;
}

/***********************************************************************
 *
 * FUNCTION:	E820Sort
 *
 * DESCRIPTION: Sort the E820 map, so a new more efficient map can be
 *		created later.
 *
 * PARAMETERS:	changePoint - List of change points
 *		changeNum - number of change points
 *		
 * RETURNS:	None.
 *
 ***********************************************************************/

initcode void E820Sort(struct ChangeMember* changePoint[],int changeNum)
{
	int stillChanging=1;
	int i;

	while (stillChanging)
	{
		stillChanging=0;
		for (i=1; i<changeNum; i++)
		{
			if ((changePoint[i]->addr < changePoint[i-1]->addr) ||
				((changePoint[i]->addr == changePoint[i-1]->addr) &&
				(changePoint[i]->addr == changePoint[i]->pBios->baseAddr) &&
				(changePoint[i-1]->addr != changePoint[i-1]->pBios->baseAddr)))
			{
				struct ChangeMember* changeTemp;
				changeTemp=changePoint[i];
				changePoint[i]=changePoint[i-1];
				changePoint[i-1]=changeTemp;
				stillChanging=1;
			}
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:	E820CreateMap
 *
 * DESCRIPTION: Create a new list of E820 entries from the change points.
 *
 * PARAMETERS:	changePoint - List of change points
 *		changeNum - number of change points
 *
 * RETURNS:	None.
 *
 ***********************************************************************/

initcode void E820CreateMap(struct ChangeMember* changePoint[], DWORD changeNum)
{
	struct MbMemoryMap* overlapList[32];
	DWORD overlapCount=0,currentType=0,lastType=0,lastAddr=0;
	DWORD changeIndex,i;

	/* Now with that sorted, create a new E820 memory map */

	for (changeIndex=0; changeIndex<changeNum; changeIndex++)
	{
		/* Detect overlapping BIOS entries */
		if (changePoint[changeIndex]->addr == changePoint[changeIndex]->pBios->baseAddr)
		{
			overlapList[overlapCount++]=changePoint[changeIndex]->pBios;
		}else{
			for (i=0; i<overlapCount; i++)
				if (overlapList[i] == changePoint[changeIndex]->pBios)
					overlapList[i]=overlapList[overlapCount-1];
			overlapCount--;
		}

		/* Now deal with those overlapping BIOS entries */
		currentType=0;
		for (i=0; i<overlapCount; i++)
		{
			if (overlapList[i]->type > currentType)
				currentType=overlapList[i]->type;
		}

		if (currentType != lastType)
		{
			if (lastType)
			{
				entries[entryCount].length=changePoint[changeIndex]->addr-lastAddr;
				if (entries[entryCount].length)
					if (++entryCount >= 32)
						break;
			}
			
			if (currentType)
			{
				entries[entryCount].base=changePoint[changeIndex]->addr;
				entries[entryCount].rangeType=currentType;
				lastAddr=changePoint[changeIndex]->addr;
			}

			lastType=currentType;
		}
	}
}

/***********************************************************************
 *
 * FUNCTION:	E820CheckEntries
 *
 * DESCRIPTION: Print the list of E820Entrys. Called by ReadE820.
 *
 * PARAMETERS:	entries - list of E820 entries.
 *		entryCount - size of E820 entry list.
 *
 * RETURNS:	1 on error (for errornous E820 entries), 0 otherwise.
 *
 ***********************************************************************/

initcode static int E820CheckEntries(struct MbMemoryMap* memEntries, int numEntries)
{
	int i;

	/* Check for entry integrity throughout the list before parsing it. */
	for (i=0; i<numEntries; i++)
	{
		if (memEntries[i].baseAddr + memEntries[i].length < memEntries[i].baseAddr)
		{
			KePrint("Weird E820 entry. Index is %d\n",i);
			return 1;
		}
	}

	return 0;
}

/***********************************************************************
 *
 * FUNCTION:	PrintE820
 *
 * DESCRIPTION: Print the list of E820Entrys. Called by ReadE820.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:	None.
 *
 ***********************************************************************/

initcode static void PrintE820()
{
	struct E820Entry* currEntry;
	int i;

	KePrint(KERN_INFO "PHYS: Number of E820 entries = %u\n", entryCount);

	for (i=0; i<entryCount; i++)
	{
		currEntry=&entries[i];
		KePrint(KERN_INFO "Start: %#016X  End: %#016X  Type: ",(DWORD)currEntry->base,(DWORD)(currEntry->base+currEntry->length));
		
		switch (currEntry->rangeType)
		{
		case 1:
			KePrint("free memory\n");
			break;
		case 2:
			KePrint("reserved\n");
			break;
		case 3:
			KePrint("ACPI Reclaim\n");
			break;
		case 4:
			KePrint("ACPI NVS\n");
			break;
		default:
			KePrint("Type = %u\n",currEntry->rangeType);
		}
	}
}

initcode static void PhysReadMemoryMap()
{
	struct ChangeMember changePointList[64];
	struct ChangeMember* changePoint[64];
	int changeNum;

	if (numEntries <= 1)
		return; /* No need to reshuffle if there's only one entry */
		
	if (E820CheckEntries(memoryMap, numEntries))
		return;

	changeNum=SetupE820Sort(changePoint, changePointList, memoryMap, numEntries);

	/* Sort the memory map from low to high */
	E820Sort(changePoint,changeNum);

	/* and create a new one from the sorted list. */
	E820CreateMap(changePoint,changeNum);

	/* Print completed entries to screen */
	PrintE820();
}

/***********************************************************************
 *
 * FUNCTION:	GetMaxPfn
 *
 * DESCRIPTION: Find the maximum page frame no. for the physical memory
 *				functions such as PageAlloc() et al.
 *
 * PARAMETERS:	None.
 *
 * RETURNS:		Maximum (free) PFN.
 *
 ***********************************************************************/

initcode static DWORD GetMaxPfn()
{
	/* 
	 * By now all the memory detection methods should have been converted to
	 * sorted E820 entries, so find the end of the highest free entry.
	 */

	DWORD retVal=0;
	int i;
	DWORD currEndPfn;
	
	/* Page-aligned start and end of the memory region. */
	QWORD start,end;

	for (i=0; i<entryCount; i++)
	{
		if (entries[i].rangeType != E820_FREE && entries[i].rangeType != E820_ACPI_RECLAIM)
			continue;

		start=PAGE_ALIGN_UP(entries[i].base);
		end=PAGE_ALIGN(entries[i].base+entries[i].length);

		/* 32-bit addresses only. */
		if (end > 0x100000000ULL)
			continue;

		if (start >= end)
			continue;

		currEndPfn = end >> PAGE_SHIFT;

		if (currEndPfn > retVal)
			retVal = currEndPfn;
	}

	return retVal;
}

/***********************************************************************
 *
 * FUNCTION:	DoReserveAreas
 *
 * DESCRIPTION: Reserve areas required by the OS.
 *
 * PARAMETERS:	maxPfn - the maximum page frame no.
 *
 * RETURNS:		Nothing.
 *
 ***********************************************************************/

DWORD ebdaAddr;

initcode static void DoReserveAreas(DWORD maxPfn)
{
	/* Reserve a few key areas. maxPfn is used because we're only allocating up to it, we don't need to reserve ACPI
	memory because it's never looked at for allocation */

	/* Reserve the first page of memory, used for BIOS data area. */
	PageReserveArea(0, PAGE_SIZE);

#if 0
	/*
	 * Reserve the extended BIOS data area. 
	 * "word at BIOS Data Area 40:0E is segment address of EBDA"
	 */

	WORD ebdaSeg=*(WORD*)PA_TO_VA(0x40E);
	if (ebdaSeg)
	{
		ebdaAddr=ebdaSeg*16;
		PageReserveArea(ebdaAddr,PAGE_SIZE);
	}
#endif

	/* Reserve BIOS code/data areas */
	PageReserveArea(0xA0000,0x100000-0xA0000);
}

initcode int PhysInit()
{
	DWORD maxPfn, memSize;
	
	PhysReadMemoryMap();

	maxPfn = GetMaxPfn();
		
	if (maxPfn >= PFN_MAX)
	{
		KePrint(KERN_INFO "PHYS: Your machine has more than 4GB of RAM. Limiting to 4GB for the moment.\n");
		maxPfn = PFN_MAX - 1;
	}
			
	PageEarlyInit(maxPfn);

	/* Print memory size for information purposes */
	memSize=(maxPfn >> 8);
	KePrint(KERN_INFO "PHYS: %umb physical memory available\n", memSize);

	if (memSize < 8)
		KernelPanic("Less than 8mb memory available. Whitix requires more than this\n");

	/* So that appropriate areas of the memory are reserved when setting up the page stack */
	DoReserveAreas(maxPfn);

	/* Set up the page allocation (memory/page_alloc.c) */
	PageInit();

	return 0;
}
