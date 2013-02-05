.align 8

.globl intSysCall
.globl endIntSysCall

intSysCall:
int $0x30
pop %ebp
ret
endIntSysCall:

.globl sysEnterSysCall
.globl endSysEnterSysCall

sysEnterSysCall:
sysenter
pop %ebp
ret
endSysEnterSysCall:
