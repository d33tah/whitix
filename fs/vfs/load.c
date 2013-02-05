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

#include <fs/vfs.h>
#include <i386/i386.h>
#include <net/socket.h>
#include <string.h>
#include <vmm.h>
#include <malloc.h>
#include <user_acc.h>
#include <elf.h>
#include <module.h>
#include <sys.h>
#include <print.h>

/* 
 * There are so many arguments to *Exec that it would be inefficent and
 * unreadable to pass them normally.
 */

#define NUM_ARG_PAGES		32
#define STACK_SIZE			32 /* megabytes */
#define LOAD_STACK_SIZE		STACK_SIZE*1024*1024

struct ExecArgs
{
	char* pathName;
	struct File exec;
	struct Process* process;
	struct Thread* thread;
	int* fds;
	char** argv;
	int argc;
	DWORD entryPoint,stackPos;
	DWORD* stackP;
	DWORD argPages[NUM_ARG_PAGES];
};

int defaultFds[]={0,1,2};

static int ExecElf(struct ExecArgs* execArgs);
static int ExecScript(struct ExecArgs* args);
static int ExecMono(struct ExecArgs* args);

/***********************************************************************
 *
 * FUNCTION: ExecOpen
 *
 * DESCRIPTION: Opens an executable file, and does basic checks for its
 *				suitability.
 *
 * PARAMETERS: pathName - file name of the process.
 *			   file - file structure.
 *
 * RETURNS:		0 if successfully opened, the return value from DoOpenFile
 *				if it failed to open, or -EISDIR if the path name is a
 *				directory.
 ***********************************************************************/

static int ExecOpen(char* pathName, struct File* file)
{
	int err;

	err=DoOpenFile(file, pathName, FILE_READ, 0);

	if (err)
		return err;
	
	if (file->vNode->mode & VFS_ATTR_DIR)
		return -EISDIR;

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: ExecCountArgs
 *
 * DESCRIPTION: Count the number of arguments in 'strings'
 *
 * PARAMETERS: strings - array containing the arguments.
 *			   max - maximum number of arguments to allow.
 *
 * RETURNS: -ENOMEM if too many arguments, 0 on success.
 *
 ***********************************************************************/

static int ExecCountArgs(char** strings, int max)
{
	int i=0;

	if (!strings)
		return 0;

	while (strings[i] != NULL)
		if (++i >= max)
			return -ENOMEM;

	return i;
}

/***********************************************************************
 *
 * FUNCTION: ExecSetupArgs
 *
 * DESCRIPTION: Copy the argument pages over.
 *
 * PARAMETERS: args - executable's arguments.
 *
 * RETURNS: -ENOMEM if memory mapping could not be allocated, 0 otherwise.
 *
 ***********************************************************************/

static int ExecSetupArgs(struct ExecArgs* args)
{
	DWORD argAddr;
	DWORD currVirtPage;
	DWORD flags;
	DWORD length=PAGE_ALIGN_UP(args->stackPos); /* Length of the mapping */
	int i=0;

	/* 
	 * Allocate 'totalPages' worth of memory as a virtual memory area. Easier to free when process exists,
	 * and MMapDo calculates the address. No faulting in though, that doesn't make sense.
	 */

	argAddr=MMapDo(args->process, NULL, 0, length, PAGE_RW | PAGE_PRESENT | PAGE_USER, 0, MMAP_PRIVATE, NULL);

	/* Failed to find a space? Unlikely. */
	if (!argAddr)
		return -ENOMEM;

	IrqSaveFlags(flags);

	/* Map in the argument pages. */
	VirtSetCurrent(args->process->memManager);

	for (currVirtPage=argAddr; currVirtPage < argAddr+length; currVirtPage+=PAGE_SIZE,i++)
		VirtMemMapPage(currVirtPage, args->argPages[i], PAGE_PRESENT | PAGE_RW | PAGE_USER);

	VirtSetCurrent(current->memManager);

	IrqRestoreFlags(flags);

	args->process->memManager->argsStart=argAddr;
	args->process->memManager->argsEnd=argAddr+length;

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: ExecCopyStrings
 *
 * DESCRIPTION: Copy the strings in 'strings' over to the argument pages.
 *
 * PARAMETERS: args - executable's arguments.
 *			   strings - array of arguments.
 *			   count - the number of arguments to copy.
 *
 * RETURNS: -EFAULT on error, 0 on success
 *
 ***********************************************************************/

static int ExecCopyStrings(struct ExecArgs* args, char** strings, int count)
{
	DWORD currVirtPage=0;
	int err=0;
	args->argc+=count;

	PreemptDisable();

	while (count-- > 0)
	{
		int len,oldLen;
		char* str=strings[count];
		int offset=0;
		int bytesToCopy;

		oldLen=len=strlen(str)+1;

		while (len > 0)
		{
			offset=args->stackPos % PAGE_SIZE;

			if (!args->argPages[args->stackPos >> 12])
			{
				if (currVirtPage) VirtUnmapPhysPage(currVirtPage);

				/* Allocate a page, and map into the list of argument pages. */
				struct PhysPage* newPage=PageAlloc();

				if (!newPage)
				{
					err=-EFAULT;
					goto out;
				}

				args->argPages[args->stackPos >> 12]=newPage->physAddr;
				currVirtPage=VirtAllocateTemp(args->argPages[args->stackPos >> 12]); /* Can stay in same address space */

				if (!currVirtPage)
				{
					err=-EFAULT;
					goto out;
				}

				ZeroMemory(currVirtPage,PAGE_SIZE);
			}else if (args->argPages[args->stackPos >> 12] && !currVirtPage)
			{
				/* Physical page has been allocated, but currVirtPage was deallocated (in a previous
				   call to ExecCopyStrings. */
				currVirtPage=VirtAllocateTemp(args->argPages[args->stackPos >> 12]);
				if (!currVirtPage)
				{
					err=-EFAULT;
					goto out;
				}
			}

			bytesToCopy=(PAGE_SIZE-offset);
			if (bytesToCopy > len)
				bytesToCopy=len;

			memcpy((void*)(currVirtPage+offset),strings[count],bytesToCopy);
			
			args->stackPos+=bytesToCopy;
			str+=bytesToCopy;
			len-=bytesToCopy;
		}
	}

out:
	if (currVirtPage)
		VirtUnmapPhysPage(currVirtPage);

	PreemptEnable();
	return 0;
}

/***********************************************************************
 *
 * FUNCTION: ExecCopyArgs
 *
 * DESCRIPTION: Copy command-line arguments.
 *
 * PARAMETERS: process - the process structure.
 *			   thread - the thread structure.
 *			   argv - list of arguments.
 *
 ***********************************************************************/

static int ExecCopyArgs(struct ExecArgs* args)
{
	int argCount;
	
	/* FIXME: Should be less than PAGE_SIZE*NUM_ARG_PAGES? */
	argCount=ExecCountArgs(args->argv, PAGE_SIZE*NUM_ARG_PAGES);
	if (argCount < 0)
		return argCount;
		
	if (args->argv)
		ExecCopyStrings(args, args->argv, argCount);

	ExecCopyStrings(args, &args->process->name, 1);

	return ExecSetupArgs(args);
}

/***********************************************************************
 *
 * FUNCTION: ExecSetupFs
 *
 * DESCRIPTION: Sets up the new process' view of the filesystem.
 *
 * PARAMETERS: execArgs - the process' creation arguments.
 *
 ***********************************************************************/

/* TODO: Move to generic load file. */

static int ExecSetupFs(struct Process* process, int* fds)
{
	/* Becomes a pain to read the code later in the function otherwise. */
	struct File* stdout=current->files[fds[0]];
	struct File* stdin=current->files[fds[1]];
	struct File* stderr=current->files[fds[2]];

	process->numFds=10;
	process->files=(struct File**)MemAlloc(sizeof(struct File)*process->numFds);
	process->root=current->root;
	current->root->refs++;

	/* Allocate the current directory string and sets it to the default value. */
	process->sCwd=(char*)MemAlloc(PATH_MAX);
	if (!process->sCwd)
	{
		MemFree(process->files);
		return -ENOMEM;
	}

	/* The child processes have the same current directory as the parent */ 
	strncpy(process->sCwd, current->sCwd, PATH_MAX);

	process->cwd=current->cwd;
	current->cwd->refs++;

	/* Copy what's in file pointers over to array */
	VfsFileDup(&stdout, &process->files[0]);
	VfsFileDup(&stdin, &process->files[1]);
	VfsFileDup(&stderr, &process->files[2]);

	return 0;
}

static void ExecSetupContext(struct ExecArgs* execArgs,char* pathName,int* fds,char** argv)
{
	/* Set up the execArgs struct that is passed to most Exec* functions */
	execArgs->pathName=pathName;
	execArgs->fds=fds;
	execArgs->argv=argv;
	execArgs->argc=0;

	ZeroMemory(execArgs->argPages, sizeof(DWORD)*NUM_ARG_PAGES);
	execArgs->stackPos=0;
}

static int ExecSetupProcess(struct ExecArgs* execArgs,char* pathName,struct Process** process)
{
	*process=ThrCreateProcess(pathName);
	if (!process)
		return -EFAULT;

	(*process)->exec=execArgs->exec.vNode;
	execArgs->process=(*process);
	(*process)->memManager->start=~0;
	(*process)->memManager->end=0;

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: Exec
 *
 * DESCRIPTION: Sets up a process with the image given by pathName.
 *
 * PARAMETERS: pathName - image of the process.
 *			   fds - standard file descriptors for the new process.
 *			   argv - list of argument strings.
 *
 ***********************************************************************/

int Exec(char* pathName, int* fds, char** argv)
{
	int err;
	struct ExecArgs execArgs;
	struct Process* process;

	ExecSetupContext(&execArgs,pathName,fds,argv);

	/* Open executable */
	err=ExecOpen(pathName,&execArgs.exec);
	if (err)
		return err;

	/* Create process */
	if (ExecSetupProcess(&execArgs,pathName,&process))
	{
		DoCloseFile(&execArgs.exec);
		return -EFAULT;
	}

	/* Copies arguments to physical pages, then maps them into memory using MMapDo. 
	   Doesn't setup argv though. */
	err=ExecCopyArgs(&execArgs);

	if (err)
		return err;

	err=ExecSetupFs(process, fds);

	if (err)
		return err;

	/* Try executing it as a script first. */
	err = ExecMono(&execArgs);
	
	if (err)
	{
		err=ExecScript(&execArgs);
		
		if (err)
			err=ExecElf(&execArgs);
	}

	if (err)
	{	
		/* Do resource cleanup */
		MmapProcessRemove(process);
		ThrPutProcess(process);
		DoCloseFile(&execArgs.exec);
		return err;
	}

	/*
	 * Now create the thread as the entry point has been given, 
	 * and the stack pointer is the top of the arguments pages.
	 */

	execArgs.thread=ThrCreateThread(process,execArgs.entryPoint, true, (DWORD)execArgs.stackP, ((void*)-1)); /* 'process' still points to the same structure */

	ThrStartThread(execArgs.thread);

	return process->pid;
}

SYMBOL_EXPORT(Exec);

static int ExecAllocateStack(struct ExecArgs* args)
{
	if (!MMapDo(args->process, NULL, ARCH_STACK_TOP - LOAD_STACK_SIZE,
		LOAD_STACK_SIZE, PAGE_RW | PAGE_PRESENT | PAGE_USER, 0, 
			MMAP_PRIVATE | MMAP_FIXED, NULL))
		return -EFAULT;

	args->stackP=(DWORD*)ARCH_STACK_TOP;

	return 0;
}

#define STACK_PUSH(stackP,item) (*(--stackP)=(DWORD)item)

int ExecSetupStack(struct ExecArgs* args)
{
	DWORD argv;
	DWORD addr;
	int argCount=args->argc;
	int len;

	addr=args->process->memManager->argsStart;

	/* Push argv entries onto stack */
	while (argCount--)
	{
		STACK_PUSH(args->stackP, addr);
		len=strlen((char*)addr)+1;
		addr+=len;
	}

	argv=(DWORD)args->stackP;

	STACK_PUSH(args->stackP,argv);
	STACK_PUSH(args->stackP,args->argc);

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: ElfMMap
 *
 * DESCRIPTION: Memory maps in an ELF segment.
 *
 * PARAMETERS: process - process in question.
 *			   header - header to be mapped in.
 *			   elfProt - area protections
 *			   mmapFlags - flags to be passed to MMapDo
 *
 ***********************************************************************/

static DWORD ElfMMap(struct Process* process,struct VNode* vNode,struct ElfSegmentHeader* header,int* elfProt,int mmapFlags)
{
	*elfProt=PAGE_PRESENT | PAGE_USER;
	if (header->segFlags & ELF_PERMS_RW)
			*elfProt|=PAGE_RW;


	return MMapDo(process, vNode, header->virtualAddr,
		PAGE_ALIGN_UP( header->fileSize+PAGE_OFFSET(header->virtualAddr) ),
				*elfProt, header->fileOffset-PAGE_OFFSET(header->virtualAddr),
					mmapFlags, NULL);
}

static void ElfMMapBss(struct Process* process,struct VNode* vNode,struct ElfSegmentHeader* header,DWORD elfBss,int elfProt)
{
	DWORD bssSize,pageAddr,partSize,offset;

	if (header->memSize > header->fileSize)
	{
		bssSize=PAGE_ALIGN_UP(header->memSize-header->fileSize);
		pageAddr=PAGE_ALIGN_UP(header->virtualAddr+header->fileSize);

		/* Memory map the pages of the BSS. */
		if (bssSize)
			MMapDo(process, NULL, pageAddr, bssSize, elfProt, 0, MMAP_PRIVATE | MMAP_FIXED, NULL);

		/* Zero out the page remainder of the BSS (it might start before a page boundary for example), and
		   read in the part-page before the BSS. */
		if (PAGE_OFFSET(elfBss))
		{
			partSize=pageAddr-(header->virtualAddr+header->fileSize);
			offset=PAGE_ALIGN(header->fileOffset+header->fileSize);
			pageAddr=PAGE_ALIGN(header->virtualAddr+header->fileSize);
			
			MMapUnmap(process, pageAddr, PAGE_SIZE);

			/* Allocate a temporary virtual page at the top of memory to copy in the last page of data from disk,
			 * then map into the process' memory space and zero out the start of the BSS to the page boundary.
			 */
			struct PhysPage* page = PageAlloc();
			DWORD phys=page->physAddr;
			DWORD virt=VirtAllocateTemp(phys);

			PageGet(page);

			FileGenericReadPage(virt, vNode, offset);
			
			VirtUnmapPhysPage(virt);

			DWORD flags;
			struct MemManager* manager=current->memManager;
			IrqSaveFlags(flags);
			VirtSetCurrent(process->memManager);
			VirtMemMapPage(PAGE_ALIGN(header->virtualAddr+header->fileSize), phys, 7);
			ZeroMemory(header->virtualAddr+header->fileSize, partSize);
			VirtSetCurrent(manager);
			IrqRestoreFlags(flags);

			/* MMap it in - although it doesn't fault VmLookupAddress may fail because the mapping does
			not finish at the end of the page. It overlaps, but it's not a big deal. */
			MMapDo(process, NULL, pageAddr, PAGE_SIZE, elfProt, 0, MMAP_PRIVATE | MMAP_FIXED, NULL);
		}
	}
}

int ElfCreateTables(struct ExecArgs* args,struct ElfHeader* header,DWORD loadAddr)
{
	struct Process* oldCurr;
	int err;
	DWORD flags;

	/* Allocate stack, then push onto it. Must switch to the new process address space
	   so pages can be faulted in. */
	err=ExecAllocateStack(args);
	if (err)
		return err;

	IrqSaveFlags(flags);

	oldCurr=current;
	current=args->process;

	VirtSetCurrent(args->process->memManager);

	/* The dynamic linker need this information to find out the base of the linker etc. */

	PUSH_AUX_ENT(args->stackP,AT_NULL, 0); /* Marker. */
	PUSH_AUX_ENT(args->stackP,AT_PAGESZ, PAGE_SIZE);
	PUSH_AUX_ENT(args->stackP,AT_BASE, loadAddr);
	PUSH_AUX_ENT(args->stackP,AT_ENTRY, header->entryPoint);
	PUSH_AUX_ENT(args->stackP,AT_PHDR, args->process->memManager->start+header->phOffset);
	PUSH_AUX_ENT(args->stackP,AT_PHENT, header->phEntrySize);
	PUSH_AUX_ENT(args->stackP,AT_PHNUM, header->phEntries);
	PUSH_AUX_ENT(args->stackP,AT_FLAGS, 0);

	/* Seperate argv and the auxillary entries table */
	STACK_PUSH(args->stackP,0);

	ExecSetupStack(args);

	/* And restore the current process. */
	current=oldCurr;
	VirtSetCurrent(current->memManager);
	IrqRestoreFlags(flags);

	return 0;
}

/***********************************************************************
 *
 * FUNCTION: ElfLoadInterp
 *
 * DESCRIPTION: Memory maps in an ELF segment.
 *
 * PARAMETERS: pathName - pathname of interpreter.
 *			   interpAddr - returns address that interpreter is loaded to.
 *			   interpEntry - entry point of the interpreter.
 *			   process - process in question.
 *
 ***********************************************************************/

static int ElfLoadInterp(char* pathName,DWORD* interpEntry,DWORD* interpAddr,struct Process* process)
{
	BYTE* buffer;
	struct ElfHeader* header;
	int i,err=0,mmapFlags;
	DWORD elfBss=0;
	DWORD loadAddr=0,mapAddr;
	struct File file;
	int elfProt;
	struct ElfSegmentHeader* physHeaders;

	err=ExecOpen(pathName,&file);
	if (err)
		goto out;

	buffer=(BYTE*)MemAlloc(1024);

	if (!DoReadFile(&file,buffer, 1024))
	{
		err=-EIO;
		goto headerFree;
	}

	header=(struct ElfHeader*)(buffer);

	if (ElfCheckHeader(header,ELF_EXEC | ELF_DYN))
	{
		err=-EIO;
		goto headerFree;
	}

	*interpAddr=~0;

	/* Read the physical headers */
	physHeaders=(struct ElfSegmentHeader*)(buffer+header->phOffset);

	for (i=0; i<header->phEntries; i++)
	{
		if (physHeaders->type == ELF_TYPE_LOAD)
		{
			mmapFlags=MMAP_PRIVATE;

			if (loadAddr)
				mmapFlags |= MMAP_FIXED;
			
			physHeaders->virtualAddr+=loadAddr;

			if (!(mapAddr=ElfMMap(process, file.vNode, physHeaders, &elfProt, mmapFlags)))
				return -ENOMEM;

			if (!loadAddr)
				loadAddr=mapAddr-PAGE_OFFSET(physHeaders->virtualAddr);

			if (mapAddr < *interpAddr)
				*interpAddr=mapAddr;

			if (elfBss < physHeaders->virtualAddr+physHeaders->fileSize)
				elfBss=physHeaders->virtualAddr+physHeaders->fileSize;

			/* Map in the zeroed section if needed */
			ElfMMapBss(process,file.vNode,physHeaders,elfBss,elfProt);
		}

		++physHeaders;
	}

	*interpEntry=header->entryPoint+loadAddr;

headerFree:
	MemFree(buffer);
	DoCloseFile(&file);

out:
	return err;
}

static int ExecRemoveFirstArg(struct ExecArgs* args)
{
	char* p;
	DWORD currVirtPage;
	
	if (!args->argc)
		return 0;
	
	currVirtPage=VirtAllocateTemp(args->argPages[args->stackPos >> 12]);
	
	p=(char*)(currVirtPage+PAGE_OFFSET(args->stackPos));
	
	p-=2;
	
	/* TODO: May have to go back to a previous page. */
	while (*p != '\0' && p > (char*)currVirtPage)
		p--;
		
	args->stackPos=(DWORD)(p-(char*)currVirtPage);
		
	args->argc--;
	
	return 0;
}

char* monoName = "/Applications/mono";

static int ExecMono(struct ExecArgs* args)
{
	char buffer[2];
	int err;
	struct File file;
	
	DoSeekFile(&args->exec, 0, 0);
	
	if (!DoReadFile(&args->exec, (BYTE*)buffer, 2))
		return -EIO;
		
	if (buffer[0] != 'M' && buffer[1] != 'Z')
		return -EINVAL;
		
	err = ExecOpen(monoName, &file);
	
	if (err)
		return -ENOENT;
	
	/* Change the executable to the interpreter. */
	memcpy(&args->exec, &file, sizeof(struct File));
	args->process->exec=args->exec.vNode;
	
	/* Remove the first argument. */
	ExecRemoveFirstArg(args);
	
	ExecCopyStrings(args, &args->process->name, 1);
	
	ExecCopyStrings(args, &monoName, 1);
	
	return ExecElf(args);
}

static int ExecScript(struct ExecArgs* args)
{
	char* buffer;
	char* copy;
	int err;
	char* interpName, *interpArg;
	struct File file;

	DoSeekFile(&args->exec, 0, 0);
	
	buffer=(char*)MemAlloc(512);
	if (!DoReadFile(&args->exec, (BYTE*)buffer, 512))
	{
		err=-EIO;
		goto freeBuffer;
	}

	if (buffer[0] != '#' || buffer[1] != '!')
	{
		err = -EINVAL;
		goto freeBuffer;
	}
		
	/* Parse the #! line */
	buffer[511]='\0';
	
	copy=strchr(buffer, '\n');
	
	if (!copy)
		copy=&buffer[511];
		
	*copy='\0';
	
	while (copy > buffer)
	{
		copy--;
		if ((*copy == ' ') || (*copy == '\t'))
			*copy='\0';
		else
			break;
	}
	
	for (copy=buffer+2; (*copy == ' ') || (*copy == '\t'); copy++);
	
	/* Could not find the interpreter name. */
	if (!*copy)
	{
		err = -EINVAL;
		goto freeBuffer;
	}
	
	/* TODO: Close the open executable? */
		
	interpName=copy;
	interpArg=NULL;
	
	for (; *copy && (*copy != ' ') && (*copy != '\t'); copy++);
	
	while ((*copy == ' ') || (*copy == '\t'))
		*copy='\0';
		
	if (*copy)
		interpArg=copy;
		
	err=ExecOpen(interpName, &file);
	
	if (err)
		goto freeBuffer;
	
	/* Change the executable to the interpreter. */
	memcpy(&args->exec, &file, sizeof(struct File));
	args->process->exec=args->exec.vNode;
	
	/* Remove the first argument. */
	ExecRemoveFirstArg(args);
	
	/* <interp> <interp args> <script name>. Done in reverse order. */
	
	ExecCopyStrings(args, &args->process->name, 1);
	
	if (interpArg)
		ExecCopyStrings(args, &interpArg, 1);
	
	ExecCopyStrings(args, &interpName, 1);
	
	err=ExecElf(args);
	
freeBuffer:		
	MemFree(buffer);
	
	return err;
}

/* TODO:
   - Improve resource cleanup
*/

/***********************************************************************
 *
 * FUNCTION: ExecElf
 *
 * DESCRIPTION: Loads an ELF executable.
 *
 * PARAMETERS: args - executable arguments (path name etc).
 *
 ***********************************************************************/

/* TODO: Support dynamic executables, make sure MoreCore has space for them. */

static int ExecElf(struct ExecArgs* args)
{
	BYTE* buffer;
	struct ElfHeader* header;
	int i,err=0,elfProt;
	struct ElfSegmentHeader* physHeaders;
	DWORD loadAddr=0,interpAddr=0;
	DWORD elfBss=0;
	struct Process* process=args->process;
	char* interpreter=NULL;

	DoSeekFile(&args->exec, 0, 0);

	/* Should calculate total program header size. TODO: Memory map in. See linker code. */
	buffer=(BYTE*)MemAlloc(512);
	if (DoReadFile(&args->exec, buffer, 512) <= 0)
	{
		err=-EIO;
		goto freeBuffer;
	}

	header=(struct ElfHeader*)buffer;

	if (ElfCheckHeader(header, ELF_EXEC | ELF_DYN))
	{
		err=-EINVAL;
		goto freeBuffer;
	}

	/* Load in the physical headers */
	physHeaders=(struct ElfSegmentHeader*)(buffer+header->phOffset);

	/* Check for a dynamic executable */
	for (i=0; i<header->phEntries; i++)
	{
		if (physHeaders[i].type == ELF_TYPE_INTERP)
		{
			interpreter=(char*)MemAlloc(physHeaders[i].fileSize); /* Good idea to read it all in? */
			DoSeekFile(&args->exec,physHeaders[i].fileOffset,0 /* SEEK_SET */);
			DoReadFile(&args->exec,(BYTE*)interpreter,physHeaders[i].fileSize);
			if (ElfLoadInterp(interpreter,&loadAddr,&interpAddr,process))
			{
				err=-EINVAL;
				goto freeBuffer;
			}

			MemFree(interpreter);
			break;
		}
	}

	/* Read off into the new processes address space. */

	for (i=0; i<header->phEntries; i++)
	{
		if (physHeaders->type == ELF_TYPE_LOAD)
		{
			/* Update the start and end of the image. */
			if (physHeaders->virtualAddr < process->memManager->start)
				process->memManager->start=physHeaders->virtualAddr;

			if (physHeaders->virtualAddr+physHeaders->memSize > process->memManager->end)
				process->memManager->end=physHeaders->virtualAddr+physHeaders->memSize;

			/* Map the section in. */
			if (!ElfMMap(process, process->exec, physHeaders, &elfProt, MMAP_PRIVATE | MMAP_FIXED))
			{
				err=-ENOMEM;
				goto freeBuffer;
			}

			/* Map the BSS in, if it exists at this point. */
			if (elfBss < physHeaders->virtualAddr+physHeaders->fileSize)
				elfBss=physHeaders->virtualAddr+physHeaders->fileSize;

			ElfMMapBss(process,args->exec.vNode,physHeaders,elfBss,elfProt);
		}
		
		physHeaders++;
	}

	args->entryPoint=loadAddr ? loadAddr : header->entryPoint;

	process->memManager->start=PAGE_ALIGN(process->memManager->start);
	process->memManager->end=PAGE_ALIGN_UP(process->memManager->end);

	ElfCreateTables(args, header, interpAddr);

freeBuffer:
	MemFree(buffer);
	return err;
}

/***********************************************************************
 *
 * FUNCTION: SysCreateProcess
 *
 * DESCRIPTION: Creates a new process.
 *
 * PARAMETERS: pathName - the filename of the executable.
 *			   fds - File descriptor values for stdin, stdout and stderr.
 *					They must be currently opened by the current process).
 *			   argv - command-line arguments for the new process.
 *
 ***********************************************************************/

int SysCreateProcess(char* pathName,int* fds,char* argv[])
{
	char* copiedName;
	int err=0;

	if (fds)
	{
		if (VirtCheckArea(fds, sizeof(int)*3, VER_READ))
			return -EFAULT;
	}else
		/* Use defaults of parent process, which is 0,1,2 */
		fds=defaultFds;

	copiedName = GetName(pathName);
	err = PTR_ERROR(copiedName);

	if (copiedName)
	{
		err=Exec(copiedName, fds, argv);
		MemFree(copiedName);
	}

	return err;
}

void LoadReleaseFsContext()
{
	int i;

	/* Now release the filesystem context */

	if (current->files)
		/* Close all files associated with the process */
		for (i=0; i<current->numFds; i++)
			DoCloseFile(current->files[i]);

	/* Does not apply to the KernelLoading process */
	if (current->exec)
		VNodeRelease(current->exec);

	VNodeRelease(current->cwd);
	VNodeRelease(current->root);

	MemFree(current->files);
	MemFree(current->sCwd);
}

struct SysCall loadSysCall=SysEntry(SysCreateProcess, 12);

int LoadInit()
{
	SysRegisterCall(SYS_LOAD_BASE, &loadSysCall);
	return 0;
}
