#ifndef LIBC_STDDEF_H
#define LIBC_STDDEF_H

#define NULL (void*)0
typedef unsigned long ptrdiff_t;
typedef unsigned short wchar_t;
typedef unsigned int size_t;
typedef signed int ssize_t;

#ifndef offsetof
#define offsetof(s,m) (size_t)&(((s *)0)->m)
#endif

#endif
