.text
.align 4

.globl DlResolve
.type DlResolve,@function

DlResolve:
	pusha
	lea 0x20(%esp),%eax
	pushl 4(%eax)
	pushl (%eax)

	call .L24
	.L24:
	popl %ebx
	addl $_GLOBAL_OFFSET_TABLE_+[.-.L24],%ebx
	mov DlResolver@GOT(%ebx),%ebx
	call *%ebx

	movl %eax,0x28(%esp)
	addl $0x8,%esp
	popa
	ret $4
