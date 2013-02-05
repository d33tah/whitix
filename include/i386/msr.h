#ifndef I386_MSR_H
#define I386_MSR_H

/* List of MSRs used by Whitix. */
#define MSR_SYSENTER_CS		0x174
#define MSR_SYSENTER_ESP	0x175
#define MSR_SYSENTER_EIP	0x176

static inline unsigned long long MsrRead(unsigned int reg)
{
	unsigned long long ret;

	asm volatile("rdmsr" : "=A"(ret) : "c"(reg));

	return ret;
}

static inline void MsrWrite(unsigned int reg, unsigned long long val)
{
	asm volatile("wrmsr" :  : "c" (reg), "A" (val));
}

#endif
