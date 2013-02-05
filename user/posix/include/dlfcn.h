#ifndef _POSIX_DLFCN_H
#define _POSIX_DLFCN_H

void* dlopen(const char* file,int mode);
int dlclose(void* handle);
char* dlerror();
void* dlsym(void* handle,const char* name);

#define RTLD_LAZY		0x01
#define RTLD_NOW		0x02
#define RTLD_GLOBAL		0x04
#define RTLD_LOCAL		0x08

#endif

