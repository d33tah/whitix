#ifndef _POSIX_PTY_H
#define _POSIX_PTY_H

#include <termios.h>
#include <sys/types.h>

int openpty(int* amaster, int* aslave, char* name, struct termios* termp, struct winsize* winp);
pid_t forkpty(int* amaster, char* name, struct termios* termp, struct winsize* winp);

#endif
