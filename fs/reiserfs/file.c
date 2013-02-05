#include "reiserfs.h"

int ReFileRead(struct File* file, BYTE* buffer, DWORD size);

struct FileOps reFileOps=
{
	.read = ReFileRead,
	.readDir = ReReadDir,
};

int ReFileRead(struct File* file, BYTE* buffer, DWORD size)
{
	KePrint("ReFileRead\n");
	return 0;
}
