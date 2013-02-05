#ifndef SETJMP_H
#define SETJMP_H

#define BUF_ESI 0
#define BUF_EDI 1
#define BUF_ESP 2
#define BUF_EBP 3
#define BUF_EBX 4
#define BUF_EIP 5
#define BUF_EFLAGS 6

#ifndef ASSEMBLY

typedef unsigned long jmp_buf[7];

void longjmp(jmp_buf env,int val);
int setjmp(jmp_buf env);

/* FIX */
typedef unsigned long sigjmp_buf[7];
#define sigsetjmp(env,val) setjmp(env)
#define siglongjmp(env,val) longjmp(env,1)

#endif

#endif
