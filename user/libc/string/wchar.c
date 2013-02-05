#include <wchar.h>
#include <stdio.h>

int wcswidth(const wchar_t *s, size_t n)
{
	printf("wcswidth\n");
	return 0;
}

size_t mbstowcs(wchar_t *dest, const char *src, size_t n)
{
	printf("mbstowcs\n");
	return 0;
}
