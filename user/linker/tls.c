#include "elf.h"
#include "lib.h"
#include "syscalls.h"
#include "hash.h"
#include "load_lib.h"
#include "tls.h"

int tlsMaxModId = 0;

char test[8];

struct TlsIndex
{
	unsigned long module;
	unsigned long offset;
};

void* __attribute__ ((__regparm__ (1))) ___tls_get_addr(struct TlsIndex* index)
{
	struct DynThreadVector* dtv = THREAD_DTV();
	
	if (index->module != 2)
	{
//		link_puts("__tls_get_addr\n");
//		link_printhex(" = ", index->module);
//		link_printhex(", = ", index->offset);
		SysExit(0);
	}
	
//	link_printhex("m = ", index->module);
//	link_puts("\n");
//	link_printhex("offset = ", index->offset);
//	link_puts("\n");
	
	return test + index->offset;
}

int DlTlsNewId()
{
	return ++tlsMaxModId;
}

unsigned long DlTlsGetSymAddr(struct ElfResolve* entry, struct ElfSym* sym)
{
	struct TlsIndex index;
	
	index.module = entry->tlsModId;
	index.offset = sym->value;
	
	return ___tls_get_addr(&index);
}

void DlTlsCreateContext()
{
	/* Set up the TLS for the thread. */
	struct ThreadCb* mainThreadCb;
	
	char* test = (char*)Dl_malloc(sizeof(struct ThreadCb)+0x20);
	mainThreadCb=(struct ThreadCb*)(test+0x20);
	mainThreadCb->threadId = SysGetCurrentThreadId();
	char* another= (char*)Dl_malloc(20);
	
//	link_memset(test, 0, sizeof(struct ThreadCb) + 0x20);
	
	mainThreadCb->vector = mainThreadCb;
	
	int seg = SysSetThreadArea(mainThreadCb);
	
	if (seg < 0)
	{
		link_puts("Could not set up TLS for initial thread. Exiting\n");
		SysExit(0);
	}
	
	GS_SET(seg);
}

int TlsInit()
{
	DlTlsCreateContext();	
	return 0;
}
