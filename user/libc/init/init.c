#include <stdio.h>
#include <stdlib.h>

FILE _stdout={
	.fd = 0,
	.flags = 0,
	.position = 0,
	.buf = 0,
};

FILE* stdout=&_stdout;

FILE _stdin={
	.fd = 1,
	.flags = 0,
	.position = 0,
	.buf = 0,
};

FILE* stdin=&_stdin;

FILE _stderr={
	.fd = 2,
	.flags = 0,
	.position = 0,
	.buf = 0,
};

FILE* stderr=&_stderr;

void _start(int argc,char* argv[])
{
	extern int main(int argc,char* argv[]);
	exit(main(argc,argv));
}
