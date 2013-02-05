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

#ifndef STRING_H
#define STRING_H

#include <stdarg.h>
#include <typedefs.h>

int strncpy(char* dest,char* src,int len);
int strncmp(const char* s1,const char* s2,int num);
int strlen(char* str);
static inline int strcmp(const char* s1,const char* s2) {return strncmp(s1,s2,0xFFFFFFFF);}
int strnicmp(const char* s1,const char* s2,int num);
char* strcat(char* s1,const char* s2);
char* strchr(char* string,int c);
void* memcpy(void* dest,void* src,long size);

#if 0
static inline void* memcpy(void* dest,void* src,long size)
{
	return ((__builtin_constant_p(size) ? __builtin_memcpy(dest,src,size) : var_memcpy(dest,src,size)));
}
#endif

void* memset(void* pointer, unsigned int size,char value);
void* memsetw(void* pointer,int size /* in words */,WORD value);
int memcmp(const void* s1, const void* s2, int n);
#define ZeroMemory(p,sz) memset((char*)(p),(sz),0)

int vsprintf(char* buf,const char* fmt, VaList args);
int vsnprintf(char* buf, int size, const char* fmt, VaList args);
int toupper(char c);

static inline int isupper(char c)
{
	return (c >= 'A' && c <= 'Z');
}

static inline int tolower(char c)
{
	if (isupper(c))
		c|=0x20;

	return c;
}

char* vasprintf(const char* fmt, VaList args);

#endif
