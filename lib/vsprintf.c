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

/* TODO: Edit this to make it cleaner */

#include <module.h>
#include <string.h>
#include <typedefs.h>
#include <malloc.h>
#include <types.h>

#define SIGN 1
#define ZEROPAD 2
#define PLUS 4
#define SPACE 8
#define LEFT 16
#define SPECIAL 32
#define SMALL 64

#define is_digit(c) ((c) >= '0' && (c) <= '9')

#define do_div(n,base) ({ \
	unsigned long __upper, __low, __high, __mod, __base; \
	__base = (base); \
	asm("":"=a" (__low), "=d" (__high):"A" (n)); \
	__upper = __high; \
	if (__high) { \
		__upper = __high % (__base); \
		__high = __high / (__base); \
	} \
	asm("divl %2":"=a" (__low), "=d" (__mod):"rm" (__base), "0" (__low), "1" (__upper)); \
	asm("":"=A" (n):"a" (__low),"d" (__high)); \
	__mod; \
})

int skip_atoi(const char** s)
{
	int i=0;
	while (is_digit(**s))
		i=i*10+*((*s)++)-'0';
	return i;
}

char* number(char* str, char* end, unsigned long long num,int base,int size,int precision,int type)
{
	char c,sign,tmp[36];
	const char* digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i=0;
	
	if (type & SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz";
	if (type & LEFT) type &= ~ZEROPAD;
	
	if (base < 2 || base > 36)
		return NULL;
		
	c=(type & ZEROPAD) ? '0' : ' ';
	
	if (type & SIGN && (signed long long)num < 0)
	{
		sign='-';
		num= -(signed long long)num;
	}else
		sign=(type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
		
	if (sign)
		size--;
	
	if (type & SPECIAL)
	{
		if (base == 16) size-=2;
		else if (base == 8) size--;
	}
		
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
	{
		tmp[i++]=digits[do_div(num,base)];
	}

	if (i>precision)
		precision=i;
		
	size-=precision;
	
	if (!(type & (ZEROPAD+LEFT)))
		while (size-- > 0)
		{
			if (str < end)
				*str = ' ';
			str++;
		}
			
	if (sign)
	{
		if (str < end)
			*str = sign;
		
		str++;
	}
	
	if (type & SPECIAL && str < end)
	{
		if (base == 8)
		{
			if (str < end)
				*str = '0';
			str++;
		}else if (base == 16)
		{
			if (str < end)
				*str='0';
			str++;
			
			if (str < end)
				*str = digits[33];
				
			str++;
		}
	}
		
	if (!(type & LEFT))
		while (size-- > 0)
		{
			if (str < end)
				*str = c;
			str++;
		}
			
	while (i < precision--)
	{
		if (str < end)
			*str='0';
			
		str++;
	}
	
	while (i-- > 0)
	{
		if (str < end)
			*str = tmp[i];
		
		str++;
	}
	
	while (size-- > 0)
	{
		if (str < end)
			*str = ' ';
			
		str++;
	}
		
	return str;
}

int vsnprintf(char* buf, int size, const char* fmt, VaList args)
{
	char* str=buf;
	char* end;
	int fieldWidth,precision,qualifier,flags;

	end = buf+size;
	
	if (end < buf)
	{
		end = ((void*)-1);
		size = end - buf;
	}

	for (; *fmt; fmt++)
	{
		fieldWidth=precision=qualifier=-1;
		flags=0;

		if (LIKELY(*fmt != '%'))
		{
			if (str < end)
				*str = *fmt;
				
			str++;
			continue;
		}
		
		repeat:
		++fmt;
		switch (*fmt)
		{
			case '-': flags |= LEFT; goto repeat;
			case '+': flags |= PLUS; goto repeat;
			case ' ': flags |= SPACE; goto repeat;
			case '#': flags |= SPECIAL; goto repeat;
			case '0': flags |= ZEROPAD; goto repeat;
		}
		
		/* Deal with the field width */
		if (is_digit(*fmt))
			fieldWidth=skip_atoi(&fmt);
		else if (*fmt == '*')
		{
			fieldWidth=VaArg(args, int);
			if (fieldWidth < 0)
			{
				fieldWidth=-fieldWidth;
				flags |= LEFT;
			}
		}
		
		if (*fmt == '.')
		{
			++fmt;
			if (is_digit(*fmt))
				precision=skip_atoi(&fmt);
			else if (*fmt == '*')
			{
				precision=VaArg(args,int);
			}
			if (precision < 0)
				precision=0;
		}
		
		qualifier=-1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
		{
			qualifier=*fmt;
			++fmt;
		}
		
		unsigned long long num=0;
		
		switch (*fmt)
		{
		case 'c':
			if (!(flags & LEFT))
				while (--fieldWidth > 0)
				{
					if (str < end)
						*str=' ';
						
					++str;
				}
				
			if (str < end)
				*str=(unsigned char)VaArg(args,int);
				
			str++;
				
			while (--fieldWidth > 0)
			{
				if (str < end)
					*str=' ';
					
				str++;
			}		
			break;

		case 's':
		{
			char* s=VaArg(args,char*);
			if (!s)
				s="<NULL>";
			int len=strlen(s);
			if (precision < 0)
				precision=len;
			else if (len > precision)
				len=precision;
				
			if (!(flags & LEFT))
				while (len < fieldWidth--)
				{
					if (str < end)
						*str=' ';
						
					str++;
				}
			int i;
			for (i=0; i<len; i++)
			{
				if (str < end)
					*str=*s;
					
				str++;
				s++;
			}
				
			while (len < fieldWidth--)
			{
				if (str < end)			
					*str++=' ';
			}
			break;
		}
		
		case 'o':
			str=number(str, end, VaArg(args,unsigned long),8,fieldWidth,
				precision,flags);
			break;
			
		case 'p':
			if (fieldWidth == -1)
			{
				fieldWidth=8;
				flags |= ZEROPAD;
			}
			str=number(str, end, (unsigned long)VaArg(args,void*),16,fieldWidth,
				precision,flags);
			break;
			
		case 'x':
			flags |= SMALL;
		case 'X':
			str=number(str, end, VaArg(args,unsigned long),16,fieldWidth,
				precision,flags);
			break;
		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			num = VaArg(args, unsigned int);
			if (flags & SIGN)
				num = (signed int)num;
			str=number(str, end, num, 10, fieldWidth, precision,flags);
			break;
		default:
			if (str < end)
				*str = '%';
			
			if (*fmt)
			{
				if (str < end)
					*str = *fmt;
				++str;
			}else
				--fmt;
			break;
		}
	}
	
	if (size > 0)
	{
		if (str < end)
			*str = '\0';
		else
			end[-1] = '\0';
	}
	
	return (str-buf);
}

SYMBOL_EXPORT(vsnprintf);

int vsprintf(char* buf,const char* fmt, VaList args)
{
	return vsnprintf(buf, INT_MAX, fmt, args);
}

SYMBOL_EXPORT(vsprintf);

int snprintf(char* buf, unsigned int size, const char* fmt, ...)
{
	int ret;
	VaList args;

	VaStart(args, fmt);
	ret = vsnprintf(buf, size, fmt, args);
	VaEnd(args);

	return ret;
}

SYMBOL_EXPORT(snprintf);

char* vasprintf(const char* fmt, VaList args)
{
	int length;
	char* ret;
	VaList temp;
	
	VaCopy(temp, args);
	
	length = vsnprintf(NULL, 0, fmt, temp);
	
	VaEnd(temp);
	
	ret = (char*)MemAlloc(length + 1);
	
	if (!ret)
		return NULL;
	
	vsnprintf(ret, length+1, fmt, args);

	return ret;
}

SYMBOL_EXPORT(vasprintf);

/* sprintf, used by Linux drivers mostly. */
int sprintf(char* str, const char* fmt, ...)
{
	KePrint("sprintf");
	return 0;
}

SYMBOL_EXPORT(sprintf);
