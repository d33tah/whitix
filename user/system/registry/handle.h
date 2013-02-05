#ifndef HANDLE_H
#define HANDLE_H

struct RegKeySetHandle
{
	struct RegKeySet* set;
	struct RegKeySetHandle* next;
	int handleId;
};

struct RegKeySetHandle* RegKeySetHandleCreate(char* name, struct RegKeySetHandle** list);
int RegKeySetGetHandleId(struct RegKeySetHandle* list, struct RegKeySetHandle* handle);
struct RegKeySetHandle* RegKeySetGetHandle(struct RegKeySetHandle* list, int id);

#endif
