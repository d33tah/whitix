#ifndef DISK_H
#define DISK_H

#define DISK_GET_GEOMETRY 0x00000001

struct DiskGeo
{
	DWORD heads,cylinders,sectors;
}PACKED;

#endif