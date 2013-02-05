#ifndef DEVICES_INPUT_H
#define DEVICES_INPUT_H

#include <devices/device.h>

struct InputDevice
{
	struct KeDevice device;
};

struct InputDevice* InputDeviceAdd(const char* name, struct FileOps* ops);

#endif
