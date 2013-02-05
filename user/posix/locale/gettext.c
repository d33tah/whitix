#include <stdio.h>
#include <stddef.h>

char* dgettext(const char* domainname, const char* msgid)
{
	return msgid;
}

char* gettext(const char* msgid)
{
//	printf("gettext(%s)\n", msgid);
	return msgid;
}

char* dcgettext(const char* domainname, const char* msgid, int category)
{
//	printf("dcgettext\n");
	return msgid;
}

char* bindtextdomain(const char* domainname, const char* dirname)
{
//	printf("bindtextdomain(%s, %s)\n", domainname, dirname);
	return NULL;
}

char* textdomain (const char * domainname)
{
//	printf("textdomain(%s)\n", domainname);
	return NULL;
}

char* ngettext(const char* msgid, const char* msgid_plural, unsigned long int n)
{
	printf("ngettext\n");
	exit(0);
}
