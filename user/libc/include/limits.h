#ifndef _LIMITS_H
#define _LIMITS_H

/* Defines for size of types */

#define CHAR_BIT   8
#define SCHAR_MIN -127
#define SCHAR_MAX +127
#define UCHAR_MAX  255
#define CHAR_MIN  SCHAR_MIN
#define CHAR_MAX  SCHAR_MAX
#define MB_LEN_MAX 1
#define SHRT_MIN  -32767
#define SHRT_MAX  +32767
#define USHRT_MAX  65535
#define LONG_MIN  -2147483647
#define LONG_MAX  +2147483647
#define INT_MIN   LONG_MIN
#define INT_MAX   LONG_MAX
#define ULONG_MAX 4294967295
#define UINT_MAX ULONG_MAX
#define SSIZE_MAX	INT_MAX

#endif

