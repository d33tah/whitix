#include <errno.h>
#include <signal.h>

#include <stdio.h> //TEMP

int signal(int sig, sig_t func)
{
	return 1;
}

int sigaction(int sig, const struct sigaction* act, struct sigaction* oact)
{
//	printf("sigaction\n");
	return 0;
}

unsigned int alarm(unsigned int seconds)
{
	printf("alarm(%u)\n", seconds);
	return 0;
}

int killpg(pid_t pgrp, int signal)
{
	printf("killpg\n");
	return 0;
}

const char* strsignal(int signal)
{
	printf("strsignal\n");
	return 0;
}

int raise(int sig)
{
	printf("raise(%d)\n", sig);
	return 0;
}

int bsd_signal(int signum, sig_t signal)
{
//	printf("bsd_signal\n");
	return 0;
}

int sigaddset(sigset_t* set, int signum)
{
//	printf("sigaddset\n");
	return 0;
}

int sigprocmask(int how, sigset_t* set, sigset_t* old_set)
{
//	printf("sigprocmask\n");
	return 0;
}

int sigfillset(sigset_t* set)
{
//	printf("sigfillset\n");
	return 0;
}

int sigdelset(sigset_t* set, int signum)
{
//	printf("sigdelset\n");
	return 0;
} 

int semget(key_t key, int nsems, int semflg)
{
	printf("semget\n");
	return 0;
}

int semop(int semid, struct sembuf* sops, unsigned int nsops)
{
	printf("semop\n");
	return 0;
}

int semctl(int semid, int semnum, int cmd, ...)
{
	printf("semctl\n");
	return 0;
}

key_t ftok(const char* pathname, int proj_id)
{
	printf("ftok(%s)\n", pathname);
	errno = ENOTIMPL;
	return -1;
}

int sigsuspend(const sigset_t* sig)
{
	printf("sigsuspend\n");
	return 0;
}

int sigaltstack(const stack_t *ss, stack_t *oss)
{
	printf("sigaltstack\n");
	return 0;
}
