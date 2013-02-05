#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

/* TODO:
 * o Clean up.
 * o Handle NANs and infinities.
 * o hexadecimal floats.
 */
 
#define isnan(x) ((x) != (x))

#define FLOAT_LOWER_BOUND	1e8
#define FLOAT_UPPER_BOUND	1e9

static const float powerTable[] =
{
	1e1, 1e2, 1e4, 1e8, 1e16, 1e32, 1e64, 1e128,
	1e256, 1e512, 1e1024, 1e2048, 1e4096,
};

#define POWER_TABLE_SIZE (sizeof(powerTable)/sizeof(powerTable[0]))
#define POWER_TABLE_MAX (1 << (POWER_TABLE_SIZE-1))

static char* PrintFloat(char* str, char* end, double num, int precision, int flags)
{
	int i, j, exp = 9;
	
	/* Simple. Add more features later. */
	if (num < 0)
	{
		*str++='-';
		num=-num;
	}
	
	if (num == 0)
		*str++='0';
	else
	{
		/* Scale exponent. */
		int expNeg=0;
		i=POWER_TABLE_SIZE;
		j=POWER_TABLE_MAX;
		if (num < FLOAT_LOWER_BOUND)
			expNeg=1;
			
		do
		{
			i--;
			if (expNeg)
			{
				if (num * powerTable[i] < FLOAT_UPPER_BOUND)
				{
					num*=powerTable[i];
					exp-=j;
				}
			}else{
				if (num / powerTable[i] >= FLOAT_LOWER_BOUND)
				{
					num/=powerTable[i];
					exp+=j;
				}
			}
			j>>=1;
		} while (i);
	}
	
	if (num >= FLOAT_UPPER_BOUND)
	{
		num/=powerTable[0];
		++exp;
	}

	/* Print out numbers */
	str+=9+(exp < 9); /* Digits per block */
	j=0;
	unsigned int block=(unsigned int)num;
	do
	{
		str[-(++j)]='0'+(block % 10);
		if (j == (9-exp))
			str[-(++j)]='.';
			
		block/=10;
	} while (j<9+(exp < 9));
	
	j=0;
	/* Trim trailing zeros. */
	while (str[-(++j)] == '0')
		str[-j]='\0';
		
	/* If the number ends in .0, there is no use having the . */
	if (str[-j] == '.')
		str[-j]='\0';
		
	*str='\0';
	
	return str;
}

/* From Linux. Very temporary. TODO: Refactor soon. */

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;
	
	if (!base) {
		base = 10;
		if (*cp == '0') {
				base = 8;
				cp++;
				if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
					cp++;
					base = 16;
				}
		}
	}else if (base == 16) {
		if (cp[0] == '0' && toupper(cp[1]) == 'X')
			cp += 2;
        }
        while (isxdigit(*cp) &&
               (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
                result = result*base + value;
                cp++;
        }
        if (endp)
                *endp = (char *)cp;
        return result;
}

/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long simple_strtol(const char *cp,char **endp,unsigned int base)
{
        if(*cp=='-')
                return -simple_strtoul(cp+1,endp,base);
        return simple_strtoul(cp,endp,base);
}

/**
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long long simple_strtoull(const char *cp,char **endp,unsigned int base)
{
        unsigned long long result = 0,value;

        if (!base) {
                base = 10;
                if (*cp == '0') {
                        base = 8;
                        cp++;
                        if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
                                cp++;
                                base = 16;
                        }
                }
        } else if (base == 16) {
               if (cp[0] == '0' && toupper(cp[1]) == 'X')
                       cp += 2;
       }
       while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
           ? toupper(*cp) : *cp)-'A'+10) < base) {
               result = result*base + value;
               cp++;
       }
       if (endp)
               *endp = (char *)cp;
       return result;
}

/**
* simple_strtoll - convert a string to a signed long long
* @cp: The start of the string
* @endp: A pointer to the end of the parsed string will be placed here
* @base: The number base to use
*/
long long simple_strtoll(const char *cp,char **endp,unsigned int base)
{
       if(*cp=='-')
               return -simple_strtoull(cp+1,endp,base);
       return simple_strtoull(cp,endp,base);
}

static int skip_atoi(const char **s)
{
       int i=0;

       while (isdigit(**s))
               i = i*10+ *((*s)++) - '0';
       return i;
}

/* TODO: Really needed? */
#define do_div(n,base) ({ \
	unsigned long __upper, __low, __high, __mod, __base; \
	__base = (base); \
	__mod = 0; \
	asm("":"=a" (__low), "=d" (__high):"A" (n)); \
	__upper = __high; \
	if (__high) { \
		__upper = __high % (__base); \
		__high = __high / (__base); \
	} \
	asm("divl %2":"=a" (__low), "=d" (__mod):"rm" (__base), "g" (__low), "1" (__upper)); \
	asm("":"=A" (n):"a" (__low),"d" (__high)); \
__mod; \
})

#define ZEROPAD               1 /* pad with zero */
#define SIGN                  2 /* unsigned/signed long */
#define PLUS                  4 /* show plus */
#define SPACE                8  /* space if plus */
#define LEFT                 16/* left justified */
#define SPECIAL              32 /* 0x */
#define LARGE                64 /* use 'ABCDEF' instead of 'abcdef' */

static char * number(char * buf, char * end, unsigned long long num, int base, int size, int precision, int type)
{
       char c,sign,tmp[66];
       const char *digits;
       static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
       static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
       int i;

       digits = (type & LARGE) ? large_digits : small_digits;
       if (type & LEFT)
               type &= ~ZEROPAD;
       if (base < 0 || base > 36)
               return NULL;
       c = (type & ZEROPAD) ? '0' : ' ';
       sign = 0;
       if (type & SIGN) {
               if ((signed long long) num < 0) {
                       sign = '-';
                       num = - (signed long long) num;
                       size--;
               } else if (type & PLUS) {
                       sign = '+';
                       size--;
               } else if (type & SPACE) {
                       sign = ' ';
                       size--;
               }
       }
       if (type & SPECIAL) {
               if (base == 16)
                       size -= 2;
               else if (base == 8)
                       size--;
       }
       i = 0;
       if (num == 0)
               tmp[i++]='0';
       else while (num != 0)
               tmp[i++] = digits[do_div(num,base)];
       if (i > precision)
               precision = i;
       size -= precision;
       if (!(type&(ZEROPAD+LEFT))) {
               while(size-->0) {
                       if (buf <= end)
                               *buf = ' ';
                       ++buf;
               }
       }
       if (sign) {
               if (buf <= end)
                       *buf = sign;
               ++buf;
       }
       if (type & SPECIAL) {
               if (base==8) {
                       if (buf <= end)
                               *buf = ' ';
                       ++buf;
               } else if (base==16) {
                       if (buf <= end)
                               *buf = ' ';
                       ++buf;
                       if (buf <= end)
                               *buf = digits[33];
                       ++buf;
               }
       }
       if (!(type & LEFT)) {
               while (size-- > 0) {
                       if (buf <= end)
                               *buf = c;
                       ++buf;
               }
       }
       while (i < precision--) {
               if (buf <= end)
                       *buf = ' ';
               ++buf;
       }
       while (i-- > 0) {
               if (buf <= end)
                       *buf = tmp[i];
               ++buf;
       }
       while (size-- > 0) {
               if (buf <= end)
                       *buf = ' ';
               ++buf;
       }
       return buf;
}

/**
* vsnprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @size: The size of the buffer, including the trailing null space
* @fmt: The format string to use
* @args: Arguments for the format string
*
* The return value is the number of characters which would
* be generated for the given input, excluding the trailing
* '\0', as per ISO C99. If you want to have the exact
* number of characters written into @buf as return value
* (not including the trailing '\0'), use vscnprintf. If the
* return is greater than or equal to @size, the resulting
* string is truncated.
*
* Call this function if you are already dealing with a va_list.
* You probably want snprintf instead.
*/
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
       int len;
       unsigned long long num;
       int i, base;
       char *str, *end, c;
       const char *s;

       int flags;              /* flags to number() */

       int field_width;        /* width of output field */
       int precision;          /* min. # of digits for integers; max
                                  number of chars for from string */
       int qualifier;          /* 'h', 'l', or 'L' for integer fields */
                               /* 'z' support added 23/7/19S.H.    */
                               /* 'z' changed to 'Z' --davidm 1/25/*/

       /* Reject out-of-range values early */
       if ((int) size < 0)
               return 0;
	   
	   str = buf;
       end = buf + size - 1;

       if (end < buf - 1) {
               end = ((void *) -1);
               size = end - buf + 1;
       }

       for (; *fmt ; ++fmt) {
               if (*fmt != '%') {
                       if (str <= end)
                               *str = *fmt;
                       ++str;
                       continue;
               }

               /* process flags */
               flags = 0;
               repeat:
                       ++fmt;          /* this also skips first '%' */
                       switch (*fmt) {
                               case '-': flags |= LEFT; goto repeat;
                               case '+': flags |= PLUS; goto repeat;
                               case ' ': flags |= SPACE; goto repeat;
                               case '#': flags |= SPECIAL; goto repeat;
                               case '0': flags |= ZEROPAD; goto repeat;
                       }

               /* get field width */
               field_width = -1;
               if (isdigit(*fmt))
                       field_width = skip_atoi(&fmt);
               else if (*fmt == '*') {
                       ++fmt;
                       /* it's the next argument */
                       field_width = va_arg(args, int);
                       if (field_width < 0) {
                               field_width = -field_width;
                               flags |= LEFT;
                       }
               }

               /* get the precision */
               precision = -1;
               if (*fmt == '.') {
                       ++fmt;  
                       if (isdigit(*fmt))
                               precision = skip_atoi(&fmt);
                       else if (*fmt == '*') {
                               ++fmt;
                               /* it's the next argument */
                               precision = va_arg(args, int);
                       }
                       if (precision < 0)
                               precision = 0;
               }

               /* get the conversion qualifier */
               qualifier = -1;
               if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
                   *fmt =='Z' || *fmt == 'z') {
                       qualifier = *fmt;
                       ++fmt;
                       if (qualifier == 'l' && *fmt == 'l') {
                               qualifier = 'L';
                               ++fmt;
                       }
               }

               /* default base */
               base = 10;

               switch (*fmt) {
                       case 'c':
                               if (!(flags & LEFT)) {
                                       while (--field_width > 0) {
                                               if (str <= end)
                                                       *str = ' ';
                                               ++str;
                                       }
                               }
                               c = (unsigned char) va_arg(args, int);
                               if (str <= end)
                                       *str = c;
                               ++str;
                               while (--field_width > 0) {
                                       if (str <= end)
                                               *str = ' ';
                                       ++str;
                               }
                               continue;

                       case 's':
                               s = va_arg(args, char *);
                               if (!(unsigned long)s)
                                       s = "<NULL>";

                               len = strnlen(s, precision);

                               if (!(flags & LEFT)) {
                                       while (len < field_width--) {
                                               if (str <= end)
                                                       *str = ' ';
                                               ++str;
                                       }
                               }
                               for (i = 0; i < len; ++i) {
                                       if (str <= end)
                                               *str = *s;
                                       ++str; ++s;
                               }
                               while (len < field_width--) {
                                       if (str <= end)
                                               *str = ' ';
                                       ++str;
                               }
                               continue;

                       case 'p':
                               if (field_width == -1) {
                                       field_width = 2*sizeof(void *);
                                       flags |= ZEROPAD;
                               }
                               str = number(str, end,
                                               (unsigned long) va_arg(args, void *),
                                               16, field_width, precision, flags);
                               continue;


                       case 'n':
                               /* FIXME:
                               * What does Csay about the overflow case here? */
                               if (qualifier == 'l') {
                                       long * ip = va_arg(args, long *);
                                       *ip = (str - buf);
                               } else if (qualifier == 'Z' || qualifier == 'z') {
                                       size_t * ip = va_arg(args, size_t *);
                                       *ip = (str - buf);
                               } else {
                                       int * ip = va_arg(args, int *);
                                       *ip = (str - buf);
                               }
                               continue;
                        
						case 'g':       
                       case 'f':
                       {
                       			double d=va_arg(args,double);
                       			str=PrintFloat(str, end, d, precision, flags);
                       			continue;
                       }

                       case '%':
                               if (str <= end)
                                       *str = '%';
                               ++str;
                               continue;

                               /* integer number formats - set up the flags and "break" */
                       case 'o':
                               base = 8;
                               break;

                       case 'X':
                               flags |= LARGE;
                       case 'x':
                               base = 16;
                               break;

                       case 'd':
                       case 'i':
                               flags |= SIGN;
                       case 'u':
                               break;

                       default:
                               if (str <= end)
                                       *str = '%';
                               ++str;
                               if (*fmt) {
                                       if (str <= end)
                                               *str = *fmt;
                                       ++str;
                               } else {
                                       --fmt;
                               }
                               continue;
               }
               if (qualifier == 'L')
                       num = va_arg(args, long long);
               else if (qualifier == 'l') {
                       num = va_arg(args, unsigned long);
                       if (flags & SIGN)
                               num = (signed long) num;
               } else if (qualifier == 'Z' || qualifier == 'z') {
                       num = va_arg(args, size_t);
               } else if (qualifier == 'h') {
                       num = (unsigned short) va_arg(args, int);
                       if (flags & SIGN)
                               num = (signed short) num;
               } else {
                       num = va_arg(args, unsigned int);
                       if (flags & SIGN)
                               num = (signed int) num;
               }
               str = number(str, end, num, base,
                               field_width, precision, flags);
       }
       if (str <= end)
               *str = '\0';
       else if (size > 0)
               /* don't write out a null byte if the buf size is zero */
               *end = '\0';
       /* the trailing null byte doesn't count towards the total
       * ++str;
       */
       return str-buf;
}

/**
* vscnprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @size: The size of the buffer, including the trailing null space
* @fmt: The format string to use
* @args: Arguments for the format string
*
* The return value is the number of characters which have been written into
* the @buf not including the trailing '\0'. If @size is <= the function
* returns 0.
*
* Call this function if you are already dealing with a va_list.
* You probably want scnprintf instead.
*/
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
       int i;

       i=vsnprintf(buf,size,fmt,args);
       return (i >= size) ? (size - 1) : i;
}

/**
* snprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @size: The size of the buffer, including the trailing null space
* @fmt: The format string to use
* @...: Arguments for the format string
*
* The return value is the number of characters which would be
* generated for the given input, excluding the trailing null,
* as per ISO C99.  If the return is greater than or equal to
* @size, the resulting string is truncated.
*/
int snprintf(char * buf, size_t size, const char *fmt, ...)
{
       va_list args;
       int i;

       va_start(args, fmt);
       i=vsnprintf(buf,size,fmt,args);
       va_end(args);
       return i;
}

/**
* scnprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @size: The size of the buffer, including the trailing null space
* @fmt: The format string to use
* @...: Arguments for the format string
*
* The return value is the number of characters written into @buf not including
* the trailing '\0'. If @size is <= the function returns 0. If the return is
* greater than or equal to @size, the resulting string is truncated.
*/

int scnprintf(char * buf, size_t size, const char *fmt, ...)
{
       va_list args;
       int i;

       va_start(args, fmt);
       i = vsnprintf(buf, size, fmt, args);
       va_end(args);
       return (i >= size) ? (size - 1) : i;
}

/**
* vsprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @fmt: The format string to use
* @args: Arguments for the format string
*
* The function returns the number of characters written
* into @buf. Use vsnprintf or vscnprintf in order to avoid
* buffer overflows.
*
* Call this function if you are already dealing with a va_list.
* You probably want sprintf instead.
*/
int vsprintf(char *buf, const char *fmt, va_list args)
{
       return vsnprintf(buf, (~0U)>>1, fmt, args);
}

/**
* sprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @fmt: The format string to use
* @...: Arguments for the format string
*
* The function returns the number of characters written
* into @buf. Use snprintf or scnprintf in order to avoid
* buffer overflows.
*/
int sprintf(char * buf, const char *fmt, ...)
{
       va_list args;
       int i;

       va_start(args, fmt);
       i=vsprintf(buf,fmt,args);
       va_end(args);
       return i;
}

/**
* vsscanf - Unformat a buffer into a list of arguments
* @buf:        input buffer
* @fmt:        format of buffer
* @args:       arguments
*/
int vsscanf(const char * buf, const char * fmt, va_list args)
{
       const char *str = buf;
       char *next;
       char digit;
       int num = 0;
       int qualifier;
       int base;
       int field_width;
       int is_sign = 0;

       while(*fmt && *str) {
               /* skip any white space in format */
               /* white space in format matchs any amount of
                * white space, including none, in the input.
                */
               if (isspace(*fmt)) {
                       while (isspace(*fmt))
                               ++fmt;
                       while (isspace(*str))
                               ++str;
               }

               /* anything that is not a conversion must match exactly */
               if (*fmt != '%' && *fmt) {
                       if (*fmt++ != *str++)
                               break;
                       continue;
               }

               if (!*fmt)
                       break;
               ++fmt;
               
               /* skip this conversion.
                * advance both strings to next white space
                */
               if (*fmt == '*') {
                       while (!isspace(*fmt) && *fmt)
                               fmt++;
                       while (!isspace(*str) && *str)
                               str++;
                       continue;
               }

               /* get field width */
               field_width = -1;
               if (isdigit(*fmt))
                       field_width = skip_atoi(&fmt);

               /* get conversion qualifier */
               qualifier = -1;
               if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
                   *fmt == 'Z' || *fmt == 'z') {
                       qualifier = *fmt++;
                       if (qualifier == *fmt) {
                               if (qualifier == 'h') {
                                       qualifier = 'H';
                                       fmt++;
                               } else if (qualifier == 'l') {
                                       qualifier = 'L';
                                       fmt++;
                               }
                       }
               }
               base = 10;
               is_sign = 0;

               if (!*fmt || !*str)
                       break;

               switch(*fmt++) {
               case 'c':
               {
                       char *s = (char *) va_arg(args,char*);
                       if (field_width == -1)
                               field_width = 1;
                       do {
                               *s++ = *str++;
                       } while (--field_width > 0 && *str);
                       num++;
               }
               continue;
               case 's':
               {
                       char *s = (char *) va_arg(args, char *);
                       if(field_width == -1)
                               field_width = INT_MAX;
                       /* first, skip leading white space in buffer */
                       while (isspace(*str))
                               str++;

                       /* now copy until next white space */
                       while (*str && !isspace(*str) && field_width--) {
                               *s++ = *str++;
                       }
                       *s = '\0';
                       num++;
               }
               continue;
               case 'n':
                       /* return number of characters read so far */
               {
                       int *i = (int *)va_arg(args,int*);
                       *i = str - buf;
               }
               continue;
               case 'o':
                       base = 8;
                       break;
               case 'x':
               case 'X':
                       base = 16;
                       break;
               case 'i':
                       base = 0;
               case 'd':
                       is_sign = 1;
               case 'u':
                       break;
               case '%':
                       /* looking for '%' in str */
                       if (*str++ != '%') 
                               return num;
                       continue;
               default:
                       /* invalid format; stop here */
                       return num;
               }

               /* have some sort of integer conversion.
                * first, skip white space in buffer.
                */
               while (isspace(*str))
                       str++;

               digit = *str;
               if (is_sign && digit == '-')
                       digit = *(str + 1);

               if (!digit
                   || (base == 16 && !isxdigit(digit))
                   || (base == 10 && !isdigit(digit))
                   || (base == 8 && (!isdigit(digit) || digit > '7'))
                   || (base == 0 && !isdigit(digit)))
                               break;

               switch(qualifier) {
               case 'H':       /* that's 'hh' in format */
                       if (is_sign) {
                               signed char *s = (signed char *) va_arg(args,signed char *);
                               *s = (signed char) simple_strtol(str,&next,base);
                       } else {
                               unsigned char *s = (unsigned char *) va_arg(args, unsigned char *);
                               *s = (unsigned char) simple_strtoul(str, &next, base);
                       }
                       break;
               case 'h':
                       if (is_sign) {
                               short *s = (short *) va_arg(args,short *);
                               *s = (short) simple_strtol(str,&next,base);
                       } else {
                               unsigned short *s = (unsigned short *) va_arg(args, unsigned short *);
                               *s = (unsigned short) simple_strtoul(str, &next, base);
                       }
                       break;
               case 'l':
                       if (is_sign) {
                               long *l = (long *) va_arg(args,long *);
                               *l = simple_strtol(str,&next,base);
                       } else {
                               unsigned long *l = (unsigned long*) va_arg(args,unsigned long*);
                               *l = simple_strtoul(str,&next,base);
                       }
                       break;
               case 'L':
                       if (is_sign) {
                               long long *l = (long long*) va_arg(args,long long *);
                               *l = simple_strtoll(str,&next,base);
                       } else {
                               unsigned long long *l = (unsigned long long*) va_arg(args,unsigned long long*);
                               *l = simple_strtoull(str,&next,base);
                       }
                       break;
               case 'Z':
               case 'z':
               {
                       size_t *s = (size_t*) va_arg(args,size_t*);
                       *s = (size_t) simple_strtoul(str,&next,base);
               }
               break;
               default:
                       if (is_sign) {
                               int *i = (int *) va_arg(args, int*);
                               *i = (int) simple_strtol(str,&next,base);
                       } else {
                               unsigned int *i = (unsigned int*) va_arg(args, unsigned int*);
                               *i = (unsigned int) simple_strtoul(str,&next,base);
                       }
                       break;
               }
               num++;

               if (!next)
                       break;
               str = next;
       }
       return num;
}

/**
* sscanf - Unformat a buffer into a list of arguments
* @buf:        input buffer
* @fmt:        formatting of buffer
* @...:        resulting arguments
*/
int sscanf(const char * buf, const char * fmt, ...)
{
       va_list args;
       int i;

		printf("sscanf(%s)\n", fmt);

       va_start(args,fmt);
       i = vsscanf(buf,fmt,args);
       va_end(args);
       return i;
}

int vfprintf(FILE* file,const char* format,va_list args)
{
	char buf[2048]; /* Ho hum.., dynamically allocate? */
	int num;

	if (!file)
		return -1;

	num=vsprintf(buf,format,args);

	fputs(buf,file);
	return num;
}

int vprintf(const char* str,va_list args)
{
	return vfprintf(stdout,str,args);
}

int fprintf(FILE* file,const char* format,...)
{
	va_list list;
	va_start(list,format);
	int num=vfprintf(file,format,list);
	va_end(list);
	return num;
}

int printf(const char* str,...)
{
	va_list list;
	va_start(list,str);
	int num=vprintf(str,list);
	va_end(list);
    return num;
}

int asprintf(char **strp, const char *fmt, ...)
{
	int num;
	
	/* FIXME. */
	*strp=(char*)malloc(4096);

	va_list list;
	va_start(list, fmt);
	
	num=vsprintf(*strp, fmt, list);
	va_end(list);
	
	return num;
}

int vasprintf(char** strp, const char* fmt, va_list ap)
{
	int num;
	
	/* FIXME. */
	*strp=(char*)malloc(4096);
	
	num=vsprintf(*strp, fmt, ap);
	
	return num;
}
