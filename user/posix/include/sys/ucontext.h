#ifndef _SYS_UCONTEXT_H
#define _SYS_UCONTEXT_H

typedef struct sigaltstack
{
	void* ss_sp;
	int ss_flags;
	size_t ss_size;
}stack_t;

#define REG_EIP				0
#define REG_EBP				1
#define REG_ESP				2
#define REG_EAX				3
#define REG_EBX				4
#define REG_ECX				5
#define REG_EDX				6
#define REG_ESI				7
#define REG_EDI				8
#define UCONTEXT_REG_NUM	9

struct mcontext
{
	int gregs[UCONTEXT_REG_NUM];
};

typedef struct mcontext mcontext_t;

struct ucontext
{
	struct ucontext*	uc_link;
	sigset_t			uc_sigmask;
	stack_t				uc_stack;
	mcontext_t			uc_mcontext;
};

typedef struct ucontext ucontext_t;

#endif
