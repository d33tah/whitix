#ifndef WAIT_H
#define WAIT_H

#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>

#define WNOHANG		0x1

#define WIFEXITED(status)	(0)
#define WIFSIGNALED(status) (((status) & 0x7F) && (((status) & 0x7F) < 0x7F))
#define WEXITSTATUS(status)	((status) & 0x80)
#define WTERMSIG(status) ((status) & 0x7F)

pid_t wait(int* status);
pid_t waitpid(pid_t pid, int* status, int options);
pid_t wait3(int* status, int options, struct rusage* rusage);
pid_t wait4(pid_t pid, int* status, int options, struct rusage* rusage);

#endif
