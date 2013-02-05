.file "syscall.S"
.text

#define SYSCALL(SysCallIndex,ReturnType,FunctionName,ArgBytes,Arguments...) \
	.type FunctionName,%function; \
	.globl FunctionName ;\
FunctionName: \
	push %ebp; \
	mov $SysCallIndex, %eax ;\
	mov $ArgBytes, %edx ;\
	lea 8(%esp), %ecx ;\
	mov $0xf8000000, %ebp; \
	jmpl *%ebp; \
	.align 8;

#include "../include/sysdefs.h"
