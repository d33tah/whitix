#ifndef _POSIX_ICONV_H
#define _POSIX_ICONV_H

typedef int iconv_t;

iconv_t iconv_open (const char *tocode, const char *fromcode);
#define iconv_close(cd) 0

#endif
