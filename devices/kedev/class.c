#include <devices/device.h>
#include <devices/class.h>
#include <fs/devfs.h>
#include <module.h>
#include <keobject.h>

struct KeSet classRoot;

int DevClassCreate(struct DevClass* devClass, struct KeObjType* type, char* name, ...)
{
	int err;
	
	err = KeSetCreate(&devClass->set, &classRoot, type, name);
	
	if (err)
		return err;
		
	return DevFsAddDir(&devClass->set, name);
}

SYMBOL_EXPORT(DevClassCreate);

int ClassInit()
{
	/* TODO: Create cache. */
	
	KeSetCreate(&classRoot, NULL, NULL, "Devices");

	return 0;
}
