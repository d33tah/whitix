#ifndef DEVICES_CLASS_H
#define DEVICES_CLASS_H

#include <keobject.h>

struct DevClass
{
	struct KeSet set;
};

int DevClassCreate(struct DevClass* devClass, struct KeObjType* type, char* name, ...);

static inline struct KeFsEntry* DevClassGetDir(struct DevClass* devClass)
{
	return devClass->set.object.dir;
}

#endif
