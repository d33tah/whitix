#include <sys/types.h>
#include <pwd.h>

struct passwd* getpwent(void)
{
	printf("getpwent\n");
	return NULL;
}

void setpwent(void)
{
	printf("setpwent\n");
}

void endpwent(void)
{
	printf("endpwent\n");
}

struct passwd* getpwnam(const char* name)
{
	printf("getpwnam\n");
	return NULL;
}

struct passwd* getpwuid(uid_t uid)
{
	printf("getpwuid\n");
	return NULL;
}

int getpwnam_r(const char *name, struct passwd *pwbuf,
		char *buf, size_t buflen, struct passwd **pwbufp)
{
	printf("getpwnam_r\n");
	return 0;
}

int getpwuid_r(uid_t uid, struct passwd *pwbuf,
		char *buf, size_t buflen, struct passwd **pwbufp)
{
	*pwbufp = malloc(sizeof(struct passwd));
	
	printf("uid = %#X, buf = %#X, buflen = %#X\n", uid, buf, buflen);
	
	strcpy(buf, "Matthew");
	strcpy(buf+sizeof("Matthew")+2, "/Applications/Mono");
	
	(*pwbufp)->pw_name = buf;
	(*pwbufp)->pw_passwd = buf;
	(*pwbufp)->pw_uid = uid;
	(*pwbufp)->pw_gid = 0;
	(*pwbufp)->pw_dir = buf+sizeof("Matthew")+2;
	(*pwbufp)->pw_shell = buf;
	(*pwbufp)->pw_gecos = NULL;
	
	return 0;
}

