#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

#include <stddef.h>

size_t strlen(const char* str);
int strcmp(const char* s1,const char* s2);
int strcoll(const char *s1, const char *s2);
int strncmp(const char* s1,const char* s2,size_t num);
char* strcpy(char* s1,const char* s2);
char* strncpy(char* s1,const char* s2,size_t num);
size_t strspn(const char* s1,const char* s2);
char* strchr(const char *s, int c);
char* strrchr(const char *s, int c);
void* memcpy(void* s1,const void* s2,size_t n);
void* memset(void* buffer,int c,size_t num);
int memcmp(const void* s1,const void* s2,size_t n);
void* memchr(const void* s, int c, size_t num);
void* memrchr(const void* s, int c, size_t n);
char* strtok(const char* string,const char* delimiters);
char* strcat(char* dest,const char* src);
char* strncat(char* dest,const char* src,size_t num);
char* strerror(int errnum);
void perror(const char* string);
char* strdup(const char* s1);
char* strndup(const char* s1, int n);
size_t strcspn(const char* s1,const char* s2);
size_t strnlen(const char* string,size_t len);
void* memmove(void* dest,const void* src,size_t len);
char* strstr(const char* haystack, const char* needle);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

#endif
