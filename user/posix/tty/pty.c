#include <pty.h>

int openpty(int* amaster, int* aslave, char* name, struct termios* termp, struct winsize* winp)
{
	printf("openpty\n");
	return 0;
}

pid_t forkpty(int* amaster, char* name, struct termios* termp, struct winsize* winp)
{
	printf("forkpty\n");
	return 0;
}

