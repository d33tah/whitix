/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <module.h>
#include <string.h>
#include <malloc.h>

int strncpy(char* dest,char* src, int len)
{
	while (*src != '\0' && len--)
		*dest++=*src++;

	while (len--)
		*dest++='\0';

	return 0;
}

SYMBOL_EXPORT(strncpy);

int strncmp(const char* s1,const char* s2,int num)
{
	unsigned char res=0;

    while (num)
	{
		if ((res=*s1-*s2++) || !*s1++)
			break;

		--num;
	}

	return res;
}

SYMBOL_EXPORT(strncmp);

int strnicmp(const char* s1,const char* s2,int num)
{
	unsigned char res=0;

	while (num)
	{
		char a=toupper(*s1),b=toupper(*s2);
		if ((res=a-b))
			break;

		s1++; s2++;
		--num;
	}

	return res;
}

SYMBOL_EXPORT(strnicmp);

int stricmp(const char* s1, const char* s2)
{
	unsigned char res=0;

	while (1)
	{
		char a = toupper(*s1), b = toupper(*s2);
		if (( res = a - b ))
			break;

		s1++; s2++;
	}

	return res;
}

SYMBOL_EXPORT(stricmp);

int strlen(char* str)
{
	int ret=0;

	while (*str++)
		++ret;

	return ret;
}

SYMBOL_EXPORT(strlen);

char* strcat(char *str1,const char *str2)
{
	char* target=str1;
	while (*target)
		target++;
	
	const char* src=str2;
	while((*target++=*src++) != '\0');
	
	return str1;
}

SYMBOL_EXPORT(strcat);

char* strncat(char* dest, const char* src, size_t count)
{
	char* tmp = dest;
	
	if (count)
	{
		while (*dest)
			dest++;
			
		while ((*dest++ = *src++))
		{
			if (!--count)
			{
				*dest = '\0';
				break;
			}
		}
	}
	
	return tmp;
}

SYMBOL_EXPORT(strncat);

char* strchr(char* string,int c)
{
	for (; *string; string++)
		if (*string == (char)c)
			return (char*)string;

	return 0;
}

SYMBOL_EXPORT(strchr);

char* strrchr(char* string,int c)
{
	for (string+=strlen(string); *string; string--)
		if (*string == (char)c)
			return (char*)string;

	return 0;
}

SYMBOL_EXPORT(strrchr);

void* memset(void* pointer, unsigned int size,char value)
{
	char* iterate=(char*)pointer;
	
	while (size--)
		*iterate++=value;

	return pointer;
}

SYMBOL_EXPORT(memset);

void* memsetw(void* pointer,int size,WORD value)
{
	WORD* iterate=(WORD*)pointer;

	while (size--)
		*iterate++=value;

	return pointer;
}

SYMBOL_EXPORT(memsetw);

int memcmp(const void *s1, const void *s2, int n)
{
	unsigned char* c1, *c2;
	int res=0;

	for (c1=(unsigned char*)s1, c2=(unsigned char*)s2; n; ++c1, ++c2, n--)
	{
		if ((res=*c1-*c2))
			break;
	}

	return res;
}

SYMBOL_EXPORT(memcmp);

int toupper(char ch)
{
	if (ch>=97 && ch<=122)
		ch-=0x20;
   
	return ch;  
}

SYMBOL_EXPORT(toupper);

char* strdup(char* name)
{
	char* ret;
	
	ret = MemAlloc(strlen(name) + 1);
	strncpy(ret, name, strlen(name));
	
	return ret;
}

SYMBOL_EXPORT(strdup);

/* strcpy - mainly for use by Linux drivers; not recommended. */

char* strcpy(char* dest, const char* src)
{
	char* save = dest;
	
	while (*src != '\0')
		*dest++ = *src++;
		
	*dest='\0';
	
	return save;
}

SYMBOL_EXPORT(strcpy);
