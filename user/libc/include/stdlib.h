#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

#include <limits.h>
#include <stddef.h>

void* malloc(size_t size);
void free(void* pointer);
void* realloc(void* mem,size_t size);
void* calloc(size_t num,size_t size);

typedef struct
{
	int quot,rem;
}div_t;

typedef struct
{
	long int quot,rem;
}ldiv_t;

div_t div(int numer,int denom);
ldiv_t ldiv(long numer,long denom);
int atoi(const char* s);
void abort(void);

int system(const char* command);

/* Should be function? */
#define abs(n) ((n < 0) ? -n : n)
#define labs(n) abs(n)

extern char** environ;

char* getenv(char* name);
int putenv(const char* name);

int unsetenv(const char* name);

void exit(int returnCode);
int atexit(void (*function)(void));

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

float strtof(const char* nptr,char** endptr);
long double strtold(const char* nptr,char** endptr);
unsigned long int strtoul(const char *nptr, char **endptr, int base);
unsigned long long int strtoull(const char *nptr, char **endptr, int base);
long int strtol(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
long long int strtoll(const char *nptr, char **endptr, int base);

void qsort(void* base,size_t num,size_t width,int (*fncompare)(const void*,const void*));

int mkstemp(char *template);

#define DBL_MIN_EXP		(-307)
#define DBL_MAX_EXP		308
#define HUGE_VAL		0x1.0p2047

#define FLT_MIN 1.175494351E-38F
#define FLT_MAX 3.402823466E+38F

#define RAND_MAX		0x7FFF

#define M_PI 3.14

/* Move */
typedef unsigned char int_fast8_t;
typedef short int int_fast16_t;
typedef long int int_fast32_t;

#define alloca(size) __builtin_alloca(size)

#endif
