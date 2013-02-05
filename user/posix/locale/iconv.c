#include <iconv.h>
#include <stddef.h>
#include <sys/types.h>

int handle=0;

iconv_t iconv_open (const char *tocode, const char *fromcode)
{
//	printf("iconv_open(%s, %s)\n", tocode, fromcode);
	return handle++;
}

size_t iconv(iconv_t cd, char** inBuf, size_t* inBytesLeft, char** outBuf, size_t* outBytesLeft)
{
	int len;

	if (!inBuf)
		return 0;

//	printf("iconv(%s, %#X, %d, %d)\n", *inBuf, *outBuf, *inBytesLeft, *outBytesLeft);

	len = strlen(*inBuf);

	/* TODO: Temp */
	strcpy(*outBuf, *inBuf);
	*inBytesLeft = 0;;
	*outBytesLeft-=len;
	*inBuf += len;
	*outBuf += len;

//	printf("inBuf = %s\n", *inBuf);

	return len;
}

char* bind_textdomain_codeset(const char* domainname, const char* codeset)
{
//	printf("bind_textdomain_codeset(%s, %s)\n", domainname, codeset);
//	return NULL;
	return "/System/Locales";
}
