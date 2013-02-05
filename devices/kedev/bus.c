#include <typedefs.h>
#include <devices/bus.h>
#include <keobject.h>

struct KeSet busRoot;

int BusInit()
{
	/* TODO: Create cache. */
	
	KeSetCreate(&busRoot, NULL, NULL, "Buses");

	return 0;
}
