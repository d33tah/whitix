.align 8

#define USER_SAVE_SREGS \
	cld; \
	push %ds; \
	push %es; \
	push %fs; \
	push %gs;
	
#define USER_SAVE_REGS \
	push %eax; \
	push %ebx; \
	push %ecx; \
	push %edx; \
	push %esi; \
	push %edi; \
	push %ebp; \
	push %esp;
	
#define USER_CALL_SAVE_REGS \
	sti; \
	pushf; \
	push %ebx; \
	push %ecx; \
	push %edx; \
	push %esi; \
	push %edi; \
	push %ebp; \
	push %esp; \
	
#define USER_INSTALL_SREGS \
	push %ax;  \
	mov $0x10,%ax; \
	mov %ax,%ds; \
	mov %ax,%es; \
	mov %ax,%fs; \
	mov %ax,%gs; \
	pop %ax

#define USER_RESTORE_REGS \
	pop %ebp; \
	pop %edi; \
	pop %esi; \
	pop %edx; \
	pop %ecx; \
	pop %ebx; \
	pop %eax; \

#define USER_CALL_RESTORE_REGS \
	pop %ebp; \
	pop %edi; \
	pop %esi; \
	pop %edx; \
	pop %ecx; \
	pop %ebx; \
	popf;

#define USER_RESTORE_SREGS \
	pop %gs; \
	pop %fs; \
	pop %es; \
	pop %ds

commonIsr:
.type  commonIsr, @function;

USER_SAVE_SREGS
USER_SAVE_REGS
USER_INSTALL_SREGS

call IrqHandle
add $0x04,%esp /* Skip over the pushed esp value */

USER_RESTORE_REGS
USER_RESTORE_SREGS

add $0x08,%esp
iretl
.size  commonIsr, .-commonIsr;
.align 8

#define errInt(irq) \
.globl int##irq; \
.type  int##irq, @function; \
int##irq: \
cld; \
pushl $0x##irq; \
jmp commonIsr; \
.size  int##irq, .-int##irq; \
.align 8

#define hwIrq(irq) \
.globl hwIrq##irq; \
.type  hwIrq##irq, @function; \
hwIrq##irq: \
cld; \
pushl $0xDEADBEEF; \
pushl $##irq; \
jmp commonIsr; \
.size  hwIrq##irq, .-hwIrq##irq; \
.align 8

#define int(irq) \
.globl int##irq; \
.type  int##irq, @function; \
int##irq: \
cld; \
pushl $0xDEADBEEF; \
pushl $0x##irq; \
jmp commonIsr; \
.size  int##irq, .-int##irq; \
.align 8

int(0)
int(1)
int(2)
int(3)
int(4)
int(5)
int(6)
int(7)
errInt(8)
int(9)
errInt(A)
errInt(B)
errInt(C)
errInt(D)
errInt(E)
int(F)
int(10)
int(11)
int(13)

hwIrq(32)
hwIrq(33)
hwIrq(34)
hwIrq(35)
hwIrq(36)
hwIrq(37)
hwIrq(38)
hwIrq(39)
hwIrq(40)
hwIrq(41)
hwIrq(42)
hwIrq(43)
hwIrq(44)
hwIrq(45)
hwIrq(46)
hwIrq(47)

.align 8

/* Common system call code. */
#define SYSCALL_COMMON \
	push %eax; \
	mov $(currThread), %eax; \
	mov (%eax), %eax; \
	mov %esp, 0x8(%eax); \
	call IrqSysCall; \
	add $0x8, %esp; \
	push %eax;              \
	mov $(thrNeedSchedule), %eax; \
	testb $0x01, (%eax); \
	jz .+0x10; \
	call PreemptCanSchedule; \
	test %eax, %eax; \
	jz .+7; \
	call ThrSchedule; \
	pop %eax;


/* INT 30 code. */
.globl intSysEntry
.type intSysEntry, @function;
intSysEntry:

USER_SAVE_SREGS
USER_CALL_SAVE_REGS
USER_INSTALL_SREGS

SYSCALL_COMMON

USER_CALL_RESTORE_REGS

/* Copy EFLAGS over, just in case we changed it in SysIoPerm. In
 * the future SysIoPerm will be deprecated. */
mov -0x4(%esp), %ecx
mov %ecx, 0x18(%esp)

USER_RESTORE_SREGS

iret

/* SYSENTER code. */
.globl SysEnterEntry
.type SysEnterEntry, @function;
SysEnterEntry:
mov (%esp), %esp /* We stored the address of the TSS esp0, so actually load the stack address here. */
USER_SAVE_SREGS
USER_CALL_SAVE_REGS
USER_INSTALL_SREGS

SYSCALL_COMMON

USER_CALL_RESTORE_REGS

USER_RESTORE_SREGS

sub $0x08, %ecx
mov $0xf8000002, %edx
sysexit

/* DispatchSysCall(DWORD numArgBytes, DWORD stackPointer, DWORD function) */
.globl DispatchSysCall;
.type  DispatchSysCall, @function;
DispatchSysCall:
push %esi
push %edi

mov 12(%esp), %eax

/* ecx has numArgBytes, edx has stackPointer */
mov %edx, %esi

/* Copy the stack over */
sub %ecx, %esp
mov %ecx, %ebp
mov %esp, %edi
rep movsb

call *%eax

add %ebp, %esp
pop %edi
pop %esi
ret
.size  DispatchSysCall, .-DispatchSysCall

/* ArchSwitchStackCall(void* stackP,void* function,void* args) */
.globl ArchSwitchStackCall
ArchSwitchStackCall:
mov 4(%esp),%eax
mov 8(%esp),%ecx
mov 12(%esp),%edx

mov %eax,%esp

pushl %edx
call *%ecx

/* Should not return */

/* i386ContextSwitch(struct Thread* old,struct Thread* new)

   The stack pointer must be the first entry in the thread structure. */
.globl i386ContextSwitch
i386ContextSwitch:

pusha

/* Actually switch the stacks */

mov 36(%esp),%eax

/* prevThread's stack may not exist if it has just been freed */
test %eax,%eax
jz popAll
mov %esp,(%eax)

popAll:
movl 40(%esp),%eax
mov (%eax),%esp

popa
ret

/* i386EnterUserSpace(DWORD entry,DWORD userStack) */
.globl i386EnterUserSpace
i386EnterUserSpace:
cli
mov $0x23,%ecx
mov %cx, %ds
mov %cx, %es
mov %cx, %fs
mov %cx, %gs

mov 4(%esp),%eax
mov 8(%esp),%ebx

pushl $0x23
pushl %ebx
pushl $(1 << 9) | 2
pushl $0x1B
pushl %eax

xorl %eax,%eax
xorl %ebx,%ebx
xorl %ecx,%ecx
xorl %edx,%edx

iret
