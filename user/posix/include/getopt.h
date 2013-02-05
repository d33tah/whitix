#ifndef _POSIX_GETOPT_H
#define _POSIX_GETOPT_H

struct option
{
	const char* name;
	int has_arg;
	int* flag;
	int val;
};

extern char* optarg;
extern int optind;
extern int opterr;
extern int optopt;

int getopt_long(int argc, char *const* argv, const char* shortopts, const struct option* longopts, int* longind);

#endif
