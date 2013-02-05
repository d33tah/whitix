#include <ctype.h>

#include <stdio.h> //TEMP

int isalnum(int c)
{
	return (isalpha(c) || isdigit(c));
}

int isalpha(int c)
{
	return (isupper(c) || islower(c));
}

int iscntrl(int c)
{
	return ((c >= 0 && c <= 31) || c == 127);
}

int isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

int islower(int c)
{
	return (c >= 'a' && c <= 'z');
}

int isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}

int isxdigit(int c)
{
	return (isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}

int isgraph(int c)
{
	return (isalpha(c) || isdigit(c));
}

int isspace(int c)
{
	return (c == ' ' || c == '\f' || c == '\t' || c == '\r' ||
	 c == '\n' || c == '\v');
}

int tolower(int c)
{
	if (isupper(c))
		c+=0x20;

	return c;
}

int ispunct(int c)
{
	return (!iscntrl(c) && !isalnum(c) && c !=' ');
}

int isprint(int c)
{
	return (isalnum(c) || ispunct(c) || isspace(c));
}

int toupper(int c)
{
	if (islower(c))
		c-=0x20;

	return c;
}

int isblank(int c)
{
	return (c == ' ' || c == '\t');
}

int isascii(int c)
{
//	printf("isascii(%c, %d)\n", c, c);
	return 1;
}
