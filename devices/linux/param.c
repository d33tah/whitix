#include <console.h>
#include <module.h>
#include <print.h>

int param_array_set(const char* val, void* kp)
{
	KePrint("param_array_set\n");
	return 0;
}

SYMBOL_EXPORT(param_array_set);

int param_set_int(const char* val, void* kp)
{
	KePrint("param_set_int\n");
	return 0;
}

SYMBOL_EXPORT(param_set_int);

int param_get_int(char* val, void* kp)
{
	KePrint("param_get_int\n");
	return 0;
}

SYMBOL_EXPORT(param_get_int);

int param_array_get(char* buffer, void* kp)
{
	KePrint("param_array_get\n");
	return 0;
}

SYMBOL_EXPORT(param_array_get);
