#include <stdlib.h>

char** environ=NULL;
char* def="";

char* getenv(char* name)
{
//	if (!strcmp(name, "LDEMULATION"))
//		return "elf_i386";
		
//	if (!strcmp(name, "MONO_LOG_LEVEL"))
//		return "debug";
	
	if (!strcmp(name, "G_DEBUG"))
		return "yes";
		
	if (!strcmp(name, "MONO_PATH"))
		return "/System/Runtime/CLR/";
		
	if (!strcmp(name, "MONO_CONFIG"))
		return "/Applications/MonoCfg/config";

	if (!strcmp(name, "MONO_DISABLE_SHM"))
		return "yes";

	if (!strcmp(name, "GC_DONT_GC"))
		return "yes";

	if (!strcmp(name, "MONO_INLINELIMIT"))
		return "0";
		
	if (!strcmp(name, "CHARSET"))
		return "UTF-8";

//	printf("getenv(%s)\n", name);
	return NULL;
}
int putenv(const char* name)
{
//	printf("putenv(%s)\n", name);
	return 0;
}
