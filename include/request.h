/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <llist.h>
#include <typedefs.h>
#include <devices/device.h>
#include <wait.h>
#include <fs/vfs.h>

#define REQUEST_READ  1
#define REQUEST_WRITE 2

struct Buffer;

struct Request
{
	int type,numSectors,retries;
	volatile BYTE* data;
	int dataLength;
	unsigned long sector;
	struct Buffer* buffer;
	struct ListHead list; /* For per-device lists */
	int ioStatus; /* -1 for complete, 0 for in progress and positive error codes */
};

/* Functions. */
struct Request* StorageBuildRequest(struct Buffer* buffer,int type);

#endif
