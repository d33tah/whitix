#ifndef STDC_STDARG_H
#define STDC_STDARG_H

typedef __builtin_va_list va_list;

/* Just get the memory location after the last argument */
#define va_start(list,lastArg) \
        __builtin_va_start(list, lastArg)

/* Must be the other way round because of the comma operator */
#define va_arg(list,type) \
        __builtin_va_arg(list, type)

#define va_copy(dst,src) __builtin_va_copy(dst, src)

/* Just for a certain cleanliness */
#define va_end(list) __builtin_va_end(list)

#endif
