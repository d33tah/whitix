#include <module.h>
#include <print.h>

int InfoInit();
int ConfigInit();

int IcInit()
{
	/* Create root directory, then add basic directories. */
	InfoInit();
	ConfigInit();
	
	return 0;
}

ModuleInit(IcInit);
