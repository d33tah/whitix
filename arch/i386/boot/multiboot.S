#define ASSEMBLY	1

#include <config.h>

/* Defines */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002
#define MULTIBOOT_HEADER_FLAGS          0x00000003

#define VA_TO_PA(addr) (addr - KERNEL_ADDRESS_BASE)

.text

.globl start

/* Sections */
.extern bss, endbss

start:
	jmp realStart
	.align 4

multiBootHeader:
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long 	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

realStart:
	cld

	/* Set up the page tables. Map in 16mb at 0x0 and 0xC0000000 */
	xorl %edx, %edx
	movl $(VA_TO_PA(bootFirstTable)), %ecx
fill:
	orl %edx, (%ecx)
	orl $0x03, (%ecx)
	addl $0x04, %ecx
	addl $0x1000, %edx
	cmp $0x1400000, %edx
	jl fill

	mov $(VA_TO_PA(bootFirstTable)), %ecx
	movl $0x0, (%ecx)

	mov $(VA_TO_PA(bootPageDirectory)), %ecx
	mov %ecx, %cr3
	mov %cr0, %ecx
	orl $0x80000000, %ecx
	mov %ecx, %cr0
	ljmp $0x08, $HigherHalfStart

HigherHalfStart:
	lgdt gdtr

	mov $(idleStack+4096), %esp
	
	/* Clear EFLAGS */
	pushl $0
	popfl

	pushl %ebx
	pushl %eax

	call i386CopyMultiBoot

	add $0x08, %esp

	mov $0x28, %ax /* TSS_SEL in the GDT */
	ltr %ax

#ifdef CONFIG_ALL_SYMBOLS
	call SymbolsCopy
#endif
	
	mov $(bss), %edi
	mov $(endbss),  %ecx
	
	subl %edi, %ecx
	xorl %eax, %eax
	shrl $0x02, %ecx
	rep stosl
	
	/* Set up the FPU */
	clts
	fninit
	
	jmp KernelMain

.data
.align 0x1000
.globl bootPageDirectory
bootPageDirectory:
	.long VA_TO_PA(bootFirstTable)+3
	.fill 767, 4, 0
	.long VA_TO_PA(bootFirstTable)+3
	.long VA_TO_PA(bootSecondTable)+3
	.long VA_TO_PA(bootThirdTable)+3
	.long VA_TO_PA(bootFourthTable)+3
	.long VA_TO_PA(bootFifthTable)+3
	.fill 250, 4, 0
	.long VA_TO_PA(bootPageDirectory)+3 /* Map directory to self. */

.data
bootFirstTable:
	.fill 1024, 4, 0
bootSecondTable:
	.fill 1024, 4, 0
bootThirdTable:
	.fill 1024, 4, 0
bootFourthTable:
	.fill 1024, 4, 0
bootFifthTable:
	.fill 1024, 4, 0

.data
gdtr:
gdtSize: .short gdtEnd-gdt-1
gdtBase: .long gdt

.globl gdt

gdt:
	.word 0, 0, 0, 0
	
	/* Code selector */
	.word 0xFFFF /* Limit 4GB */
	.word 0x0000 /* Base address is 0 */
	.word 0x9A00 /* Code, read and executable. */
	.word 0x00CF /* Granularity and limit */

	/* Data selector */
	.word 0xFFFF /* Limit 4GB */
	.word 0x0000 /* Base address is 0 */
	.word 0x9200 /* Data, read/write */
	.word 0x00CF /* Granularity and limit */
	
	/* User code selector */
	.word 0xFFFF /* Limit 4GB */
	.word 0x0000 /* Base address is 0 */
	.word 0xFA00 /* Ring 3, code, read */
	.word 0x00CF /* Granularity and limit */
	
	/* User data selector */
	.word 0xFFFF /* Limit 4GB */
	.word 0x0000 /* Base address is 0 */
	.word 0xF200 /* Ring 3, data, read/write */
	.word 0x00CF /* Granularity and limit */
	
	/* TSS selector */
	.word 0x68	/* Limit (length of TSS) */
	.word 0x2100 /* Base */
	.word 0x8900 	/* Type */
	.word 0x0000 /* Granularity and limit */

	/* TLS selector. Modified at every task switch */
	.word 0x0
	.word 0x0
	.word 0x0
	.word 0x0

gdtEnd:	
