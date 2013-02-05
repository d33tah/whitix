#define ASSEMBLY
#include <setjmp.h>

/* Don't need to access global data, so no need to access ebx */

.global longjmp
.type longjmp,%function
.align 4
longjmp:

movl 4(%esp),%ecx
movl 8(%esp),%eax

/* Now transfer all data to registers */
movl (BUF_EIP*4)(%ecx),%edx
movl (BUF_EBX*4)(%ecx),%ebx
movl (BUF_EDI*4)(%ecx),%edi
movl (BUF_ESI*4)(%ecx),%esi
movl (BUF_EBP*4)(%ecx),%ebp
movl (BUF_ESP*4)(%ecx),%esp
jmp *%edx
.size longjmp, .-longjmp

.global setjmp
.type setjmp,%function
.align 4
setjmp:
xorl %eax,%eax
movl 4(%esp),%edx

movl %ebx,(BUF_EBX*4)(%edx)
movl %esi,(BUF_ESI*4)(%edx)
movl %edi,(BUF_EDI*4)(%edx)
leal 4(%esp),%ecx
movl %ecx,(BUF_ESP*4)(%edx)
movl 0(%esp),%ecx
movl %ecx,(BUF_EIP*4)(%edx)
movl %ebp,(BUF_EBP*4)(%edx)

ret
.size setjmp, .-setjmp
