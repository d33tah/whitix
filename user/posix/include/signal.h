/* FIXME: TODO */

#ifndef POSIX_SIGNAL_H
#define POSIX_SIGNAL_H

#include <sys/types.h>
#include <sys/ucontext.h>

#define SIG_DFL 0
#define SIG_ERR 1
#define SIG_HOLD 2
#define SIG_IGN 3

#define SIG_SETMASK	0x1
#define SIG_BLOCK	0x2
#define SIG_UNBLOCK	0x4

#define FPE_INTDIV 0
#define FPE_FLTDIV 1

#define sigemptyset(set) (void)0

union sigval
{
	void* data;
};

typedef struct
{
	int si_signo;
	int si_code;
	union sigval si_value;
	int si_errno;
	pid_t si_pid;
	uid_t si_uid;
	void *si_addr;
	int si_status;
	int si_band;
}siginfo_t;

struct sigaction
{
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_sigaction)(int,siginfo_t*,void*);
};

typedef void (*sig_t)(int);

int signal(int sig, sig_t func);
int sigaction(int sig, const struct sigaction* act, struct sigaction* oact);

#define SA_SIGINFO		0x1
#define SA_RESETHAND	0x2
#define SA_RESTART		0x4
#define SA_NOCLDSTOP	0x8
#define SA_ONSTACK		0x10

/* Rejuggle so Whitix signals map to POSIX ok without having to redirect */
#define SIGABRT 0
#define SIGALRM 1
#define SIGBUS 2
#define SIGCHLD 3
#define SIGCONT 4
#define SIGFPE 5
#define SIGHUP 6
#define SIGILL 7
#define SIGINT 8
#define SIGKILL 9
#define SIGPIPE 10
#define SIGQUIT 11
#define SIGSEGV 12
#define SIGSTOP 13
#define SIGTERM 14
#define SIGTSTP 15
#define SIGTTIN 16
#define SIGTTOU 17
#define SIGUSR1 18
#define SIGUSR2 19
#define SIGPOLL 20
#define SIGPROF 21
#define SIGSYS 22
#define SIGTRAP 23
#define SIGURG 24
#define STVTALARM 25
#define SIGXCPU 26
#define SIGXFSZ 27
#define SIGPWR	28
#define SIGINFO 29

static inline int kill(pid_t pid,int sig)
{
	(void)pid;
	(void)sig;
	return 1;
}

int killpg(pid_t pgrp, int signal);

#define SS_ONSTACK 1
#define SS_DISABLE 2

#endif
