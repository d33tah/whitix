/* General file information */
.file "memcpy.S"
.text
.align 8
.globl memcpy

memcpy:
	push %esi
	push %edi
	movl 12(%esp),%edi
	movl 16(%esp),%esi
	movl 20(%esp),%ecx
	rep movsb
	pop %edi
	pop %esi
	movl 4(%esp),%eax
	ret

