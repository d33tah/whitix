#include <i386/i386.h>
#include <error.h>
#include <print.h>
#include <sys.h>
#include <task.h>
#include <typedefs.h>

void GdtSet(unsigned long* gdt, DWORD base, DWORD limit, DWORD type, DWORD flags)
{
	gdt[0] = ((base & 0xFFFF) << 16) | (limit & 0xFFFF);
	gdt[1] = (base & 0xFF000000) | ((base & 0xFF0000) >> 16)
		| (limit & 0x000F0000) | ((type & 0xFF) << 8) | ((flags & 0xF) << 20);
}

int SysSetThreadArea(DWORD base)
{
	DWORD flags;

	IrqSaveFlags(flags); /* Do we need to lock here? */

	/* Have we set a thread area already? */
	if (currThread->tlsGdt[0] || currThread->tlsGdt[1])
		return -EEXIST;

	currThread->tlsGdt[0] = ((base & 0xFFFF) << 16) | 0xFFFF;
	currThread->tlsGdt[1] = (base & 0xFF000000) | ((base & 0x00FF0000) >> 16)
		| (0xF0000) | (1 << 9) | (1 << 15) | (1 << 22) | (1 << 23) | 0x7000;

/* FIXME: Figure out flags. */
//	GdtSet(currThread->tlsGdt, base, 0xFFFFFFFF, 0x53, 0xC);

	TlsInstall(currThread);

	IrqRestoreFlags(flags);

	return GDT_TLS_SELECTOR;
}

struct SysCall tlsCalls = SysEntry(SysSetThreadArea, 4);

int TlsInit()
{
	SysRegisterCall(SYS_TLS_BASE, &tlsCalls);
	return 0;
}
