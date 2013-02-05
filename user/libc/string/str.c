#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>

size_t strlen(const char* str)
{
	size_t ret=0;

	while (*str++)
		++ret;

	return ret;
}

size_t strnlen(const char* str,size_t maxlen)
{
	const char* curr;
	for (curr=str; *curr && maxlen-- ; ++curr);
	return curr-str;
}

int strcmp(const char* s1,const char* s2)
{
	char result=0;

    while (1)
	{
		if ((result=*s1-*s2++) || !*s1++)
			break;
	}

    return result;
}

int strcoll(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}

char* strcpy(char* s1,const char* s2)
{
	char* s=s1;

	while ((*s++=*s2++) != 0);
	
	return s1;
}

char* strncpy(char* dest,const char* src,size_t count)
{
	char* s = dest;

	while (count--)
	{
		if ((*s = *src) != 0) src++;
		++s;
	}
		
	return dest;
}

char* stpcpy(char* dest, const char* src)
{
	while ( (*dest++ = *src++) != 0);
	
	return dest - 1;
}

int strncmp(const char* s1,const char* s2,size_t num)
{
	unsigned char c1, c2;

    while (num-- > 0)
	{
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		
		if (c1 != c2)
			return c1-c2;
		
		if (c1 == '\0')
			return 0;
	}

    return 0;
}

size_t strspn(const char* s1,const char* s2)
{
	size_t count=0;
	const char* orig=s1;

	for (; *s1 != '\0'; s1++)
	{
		const char* a;
		for (a=s2; *a != '\0'; a++)
			if (*a == *s1)
				break;

		if (*a == '\0')
			return count;
		else
			++count;
	}

	return s1-orig;
}

char* strchr(const char *s, int c)
{
	do
	{
		if (*s == (char)c)
			return (char*)s;
	} while(*s++);

	return NULL;
}

char* strrchr(const char *s, int c)
{
	char* end=s+strlen(s);

	do
	{
		if (*end == (char)c)
			return (char*)end;
	}while (end-- > s);

	return NULL;
}

int memcmp(const void* s1,const void* s2,size_t n)
{
	char result=0;
	char* str1=s1,*str2=s2;

	while (n--)
	{
		result=(*str1)-(*str2);

		if (result)
			return result;

		str1++;
		str2++;
	}

	return 0;
}

char* strcat(char* dest,const char* src)
{
	char* s=dest;

	while (*s++);
	--s;
	while ((*s++=*src++) != 0);

	return dest;
}

char* strncat(char* dest,const char* src,size_t num)
{
	char* s = dest;

	while (*s++);
	s--;

	while (num && ((*s = *src++)))
	{
		num--;
		++s;
	}

	*s = '\0';

	return dest;
}

/* FIXME: Not finished */
char* strtok(const char* string,const char* delimiters)
{
	printf("strtok, fix\n");
	/*static char* prevStr=NULL;

	if (!string)
	{
		if (!prevStr)
			return NULL;
		else
			string=prevStr;
	}

	char* begin=(char*)string;
	while (*string != '\0')
	{
		 Search through delimiters
		char* d;
		for (d=delimiters; *d != '\0'; d++)
		{
			if (*string == *d)
			{
				Found a delimiter, null out the current position in string
				*string='\0';
				break;
			}
		}
	}

	prevStr=string;*/
	return 0;
}

size_t strcspn(const char* s1,const char* s2)
{
	const char* p,*r;
	size_t count=0;

	for (p=s1; *p; ++p)
	{
		for (r=s2; *r; ++r)
			if (*p == *r)
				return count;

		++count;
	}

	return count;
}

char* errorTable[]={
	"No error",
	"No permission",
        "No such file or directory",
        "Low-level I/O error",
        "Bad file descriptor",
        "Out of memory",
        "Not allowed access",
        "Device is busy",
        "Bad address",
        "Already exists",
        "Out of file handles",
        "No space on device",
	"Read-only filesystem",
        "No such system call",
	"Function not implemented",	"Not a directory",
	"I/O is pending",
	"Is a directory",
	"Invalid value/operation",
	"Interrupted by signal",
};

char** sys_errlist = errorTable;

char* strerror(int errnum)
{
	if (errnum < 0)
		errnum=-errnum;

     if (errnum > EINTR)
		return "Invalid errno";

	return errorTable[errnum];
}

void perror(const char* string)
{
	fprintf(stderr,"%s: %s\n",string,strerror(errno));
}

char* strdup(const char* s1)
{
	if (!s1)
		return NULL;

	char* addr=(char*)malloc(strlen(s1)+1);
	if (!addr)
		return NULL;

	strcpy(addr,s1);
	return addr;
}

char* strndup(const char* s1, int n)
{
	char* s;

	n = strnlen(s1, n);
	
	if ( (s = malloc(n+1)) != NULL )
	{
		memcpy(s, s1, n);
		s[n] = '\0';
	}
	
	return s;
}

void* memset(void* buffer,int c,size_t num)
{
	char* dest=(char*)buffer;

	while (num--)
		*dest++=(char)c;

	return buffer;
}

void* memcpy(void* s1,const void* s2,size_t num)
{
	char* dest=(char*)s1;
	char* src=(char*)s2;

	while (num--)
		*dest++=*src++;

	return s1;
}

void* memmove(void* dest,const void* src,size_t len)
{
	char* tmp,*s;

	if (len <= 0)
		return dest;

	if (dest == src)
		return dest;

	if (dest <= src)
	{
		tmp=(char*)dest;
		s=(char*)src;
		while (len--)
			*tmp++=*s++;
	}else{
		tmp=(char*)dest+len;
		s=(char*)src+len;
		while (len--)
			*--tmp=*--s;
	}

	return dest;
}

#define TODO printf("%s\n",__func__); exit(0)

float strtof(const char* str,char** endptr)
{
	char* pos=str;
	char* pos0=NULL;
	char* posHex=NULL;
	int negative=0;
	int expTemp;
	int expPower=1;
	char expChar='c';
	float number=0;
	int pBase=10;

	while (isspace(*pos))
		pos++;

	switch (*pos)
	{
	case '-':
		negative=1;
	case '+':
		pos++;
	}

	if ((*pos == '0') && (((pos[1]) | 0x20) == 'x'))
	{
		posHex=++pos;
		++pos;
		expChar='p';
		pBase=16;
	}

loop:
	while (isxdigit(*pos))
	{
		number=(number*pBase)+(isdigit(*pos) ? (*pos-'0') : (((*pos)|0x20) -('a'-10)));
		pos++;
	}

	if (!pos0 && *pos == '.')
	{
		pos0=++pos;
		goto loop;
	}

	if (pos0)
		expPower+=(pos0-pos);

	if (posHex)
	{
		expPower*=4;
		pBase=2;
	}

	if (negative)
		number=-number;

	/* Does it have an exponent? */
	if (((*pos)|0x20) == expChar)
	{
		negative=1;
		switch (*++pos)
		{
		case '-':
			negative=-1;
		case '+':
			++pos;
		}

		pos0=pos;
		expTemp=0;
		while (isdigit(*pos))
		{
			expTemp=expTemp*10+(*pos-'0');
			++pos;
		}

		expPower+=negative*expTemp;
	}

	while (expPower)
	{
		if (expPower < 0)
		{
			number/=pBase;
			expPower++;
		}else{
			number*=pBase;
			expPower--;
		}
	}

	return number;
}

long double strtold(const char* nptr,char** endptr)
{
	TODO;
}

unsigned long int strtoul(const char *str, char **endptr, int base)
{
	int negative=0;
	unsigned long number = 0;
	
	while (isspace(str))
		str++;

	/* Optional sign */
	switch (*str)
	{
		case '-':
			negative=1;
		case '+':
			str++;
	}
	
	if (!(base & ~0x10))
	{
		base += 10;
		if (*str == '0')
		{
			base -=2;
			str++;
			
			if (tolower(*str) == 'x')
			{
				++str;
				base += base;
			}
		}
		
		if (base > 16)
			base = 16;
	}
	
	if (((unsigned)(base - 2)) < 35)
	{
		unsigned long cutOffDigit, cutOff; /* Use. ULONG_MAX */
		
		do
		{
			unsigned char digit;
			
			if ((*str - '0') <= 9)
				digit = *str - '0';
			else if (*str >= 'A')
				digit = tolower(*str)-'a'+10;
			else
				digit = 40;
			
			if (digit >= base)
				break;
				
			++str;
			
			number = number*base + digit;
		} while(1);
	}
	
	if (endptr)
		*endptr=str;
	
	return negative ? (unsigned long)(-((long)number)) : number;
}

unsigned long long int strtoull(const char *nptr, char **endptr, int base)
{
	TODO;
	while (1);
}

unsigned long int __strtoul_internal(const char * __nptr, char * * __endptr, int __base, int __group)
{
	printf("__strtoul_internal\n");
	return 0;
}

/* Rewrite really - not my code */

long int strtol(const char *nptr, char **endptr, int base)
{
	/* Skip whitespace */
	const char* s=nptr;
	int c,neg=0,any,cutlim;
	unsigned long cutoff;
	long acc;

	do{
		c=*s++;
	}while(isspace(c & 0xFF));

	if (c == '-')
	{
		neg=1;
		c=*s++;
	}else if (c == '+')
		c=*s++;

	if ((!base || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
	{
		c=s[1];
		s+=2;
		base=16;
	}

	if (!base)
		base=(c == '0') ? 8 : 10;

	cutoff=neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim=cutoff % (unsigned long)base;
	cutoff/=(unsigned long)base;
	for (acc=0,any=0; ; c=*s++, c &= 0xFF)
	{
		if (isdigit(c))
			c-='0';
		else if (isalpha(c))
			c-=isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if (c >= base)
			break;

		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any=-1;
		else
		{
			any=1;
			acc*=base;
			acc+=c;
		}
	}

	if (any < 0)
	{
		acc=neg ? LONG_MIN : LONG_MAX;
		errno=ERANGE;
	}else if (neg)
		acc=-acc;

	if (endptr != 0)
		*endptr=any ? ((char*)s) - 1 : (char*)nptr;

	return acc;
}
double __strtod_internal(const char * __nptr, char * * __endptr, int __group)
{
	printf("__strtod_internal\n");
	return 0.0;
}

double strtod(const char *str, char **endPtr)
{
	double number=0.0;
	int exponent=0, negative=0;
	double p10;
	int n=0;
	int numDigits=0, numDecimals=0;
	char* p=(char*)str;
	
	/* Skip leading whitespace in the string. */
	while (isspace(*p)) p++;
	
	/* Handle the presence of a sign */
	switch (*p)
	{
		case '-':
			negative=1;
		case '+':
			p++;
	}
	
	/* Process the string of digits that follows */
	while (isdigit(*p))
	{
		number=number*10 + (*p - '0');
		p++;
		numDigits++;
	}
	
	/* and the decimal part, if one exists. */
	if (*p == '.')
	{
		p++;
		
		while (isdigit(*p))
		{
				number=number*10 + (*p - '0');
				p++;
				numDigits++;
				numDecimals++;
		}
		
		exponent-=numDecimals;
	}
	
	if (numDigits == 0)
	{
		/* Doesn't make sense as a double. */
		errno=ERANGE;
		return 0.0;
	}
	
	/* Correct the sign */
	if (negative)
		number=-number;
		
	/* Process an exponent string. */
	if (*p == 'e' || *p == 'E')
	{
		negative=0;
		
		/* Handle (optional) sign. */
		switch (*++p)
		{
			case '-':
				negative=1;
			case '+':
				p++;
		}
		
		/* Process the string of digits that follows. */
		while (isdigit(*p))
		{
			n=n*10 + (*p - '0');
			p++;
		}
		
		/* and adjust for the sign. */
		if (negative)
			exponent-=n;
		else
			exponent+=n;
	}
	
	/* Check that exponent is not out of bounds. */
	if (exponent < DBL_MIN_EXP || exponent > DBL_MAX_EXP)
	{
		errno=ERANGE;
		return HUGE_VAL;
	}
	
	/* Scale the result. */
	p10=10.0;
	n=exponent;
	if (n < 0)
		n=-n;
		
	while (n > 0)
	{
		if (n & 1)
		{
			if (exponent < 0)
				number/=p10;
			else
				number*=p10;
		}
		
		n >>= 1;
		p10*=p10;
	}
	
	/* Finish off */
	if (number == HUGE_VAL) errno=ERANGE;
	if (endPtr) *endPtr=p;
	
	return number;
}

double atof(const char* str)
{
	return strtod(str,NULL);
}

long long int strtoll(const char *nptr, char **endptr, int base)
{
	TODO;
}

char* strstr(const char* haystack, const char* needle)
{
	const char* s=haystack;
	const char* p=needle;

	do {
		if (!*p)
			return haystack;

		if (*p == *s)
		{
			++p;
			++s;
		}else{
			p=needle;
			if (!*s)
				return NULL;

			s=++haystack;
		}
	}while(1);
}

char* strcasestr(const char* haystack, const char* needle)
{
	const char* s=haystack;
	const char* p=needle;

	do {
		if (!*p)
			return haystack;

		if (toupper(*p) == toupper(*s))
		{
			++p;
			++s;
		}else{
			p=needle;
			if (!*s)
				return NULL;

			s=++haystack;
		}
	}while(1);
}

void* memchr(const void* s, int c, size_t n)
{
	unsigned char ch=(unsigned char)c;
	unsigned char* mem=(unsigned char*)s;
	int i=0;
	while (i < n)
	{
		if (mem[i] == ch)
			return &mem[i];
		i++;
	}

	return NULL;
}

void* memrchr(const void* s, int c, size_t n)
{
	unsigned char ch=(unsigned char)c;
	unsigned char* mem=(unsigned char*)s;
	int i=n;
	
	while (i > 0)
	{
		if (mem[i] == ch)
			return &mem[i];
			
		i--;
	}

	return NULL;
}

char *strpbrk(const char *str, const char *accept)
{
	char* s, *p;

	for (s=str; *s; s++)
		for (p=accept; *p; p++)
			if (*p == *s) return s;
	return NULL;
}

/* TODO: Check. See uClibc */

int strncasecmp(const char* s1, const char* s2, size_t num)
{
	int result=0;

	while (num && ( ( s1 == s2) ||
		!(result = ((int)(tolower(*((unsigned char*)s1))))
		- tolower(*((unsigned char*)s2))))
		&& (--num, ++s2, *s1++));

	return result;
}

int strcasecmp(const char *s1, const char *s2)
{
	return strncasecmp(s1, s2, ~0);
}

int strcmpi(const char* s1, const char* s2)
{
	return strncasecmp(s1, s2, ~0);
}

size_t strxfrm(char* dest, const char* src, size_t n)
{
	printf("strxfrm\n");
	return 0;
}

char *strsep(char **stringp, const char *delim)
{
	printf("strsep\n");
	return NULL;
}

void *mempcpy(void *dest, const void *src, size_t n)
{
	printf("mempcpy\n");
	return NULL;
}

int strverscmp(const char *s1, const char *s2)
{
	printf("strverscmp\n");
	return 0;
}
