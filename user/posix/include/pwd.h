#ifndef _POSIX_PWD_H
#define _POSIX_PWD_H

#include <time.h>
#include <sys/types.h>

struct passwd
{
	char* pw_name,*pw_passwd;
	uid_t pw_uid;
	gid_t pw_gid;
	time_t pw_change;
	char* pw_class,*pw_gecos,*pw_dir,*pw_shell;
	time_t pw_expire;
};

#endif
