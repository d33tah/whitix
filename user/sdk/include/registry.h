#ifndef _REGISTRY_H
#define _REGISTRY_H

#define REG_NAME_LENGTH		1024

#define REG_KEY_TYPE_STR	0
#define REG_KEY_TYPE_INT	1

struct Registry
{
	int sockFd;
};

struct RegKeySet
{
	int handleId;
	struct Registry* registry;
};

struct RegKeySetIter
{
	struct RegKeySet* keySet;
	int bufSize;
	char* buf;
	int next, size, index;
};

int RegRegistryOpen(char* name, struct Registry* registry);
int RegKeySetOpen(struct Registry* registry, char* name, unsigned long accessRights, struct RegKeySet* keySet);
int RegKeyReadString(struct RegKeySet* keySet, char* name, char** data, unsigned long* length);
int RegKeyReadInt(struct RegKeySet* keySet, char* name, int* ret);
int RegKeySetReadKey(struct RegKeySet* keySet, char* name, int* type, char* data, unsigned long* length);

#endif
