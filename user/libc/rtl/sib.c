#include <syscalls.h>

#define SIB_USER_ADDR		0xF8001000

struct SystemInfoBlock
{
	int version;

	unsigned long seconds;
	unsigned long useconds;
}__attribute__((packed));

struct SystemInfoBlock* block = (struct SystemInfoBlock*)SIB_USER_ADDR;

int SysGetTime(void* pointer)
{
	struct Time* time = (struct Time*)pointer;

	time->seconds = block->seconds;
	time->useconds = block->useconds;

	return 0;
}
