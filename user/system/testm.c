#include <module.h>

extern int printf(char*, ...);

int TestInit()
{
	printf("Test!\n");
	return 0;
}

MODULE_INIT(TestInit);
