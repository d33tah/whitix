/***************************************************************************
    begin                : Wed Oct 8 2003
    copyright            : (C) 2003 - 2007 by Alper Akcan
    email                : distchx@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************/

#if defined(VIDEO_NULL)

#include "../../../lib/xynth_.h"
#include "server.h"
#include "null.h"

int mouseFd=0;

int s_video_null_mouse_init (s_server_conf_t *cfg)
{
	mouseFd=SysOpen("/System/Devices/Input/Ps2Mouse", _SYS_FILE_READ, 0);

	if (mouseFd < 0)
		return -1;

	/* TODO: Set file to no delay when reading. */

	return mouseFd;
}

struct Ps2PacketFlags
{
	unsigned char leftB:1;
	unsigned char rightB:1;
	unsigned char middleB:1;
	unsigned char one:1;
	unsigned char xSign:1;
	unsigned char ySign:1;
	unsigned char xOverflow:1;
	unsigned char yOverflow:1;
}__attribute((packed));

struct Ps2Packet
{
	struct Ps2PacketFlags flags;
	unsigned char dX, dY;
}__attribute((packed));

int s_video_null_mouse_update (s_video_input_data_t *mouse)
{
	struct Ps2Packet pack;
	int dX, dY;

readAgain:
	if (SysRead(mouseFd, &pack, sizeof(struct Ps2Packet)) < 0)
		return -EIO;

	if (pack.flags.xOverflow || pack.flags.yOverflow)
		goto readAgain;

//	printf("pack.flags = %u, pack.dX = %d, dY = %d\n", *(unsigned char*)&pack.flags, pack.dX, pack.dY);
//	printf("xSign = %u, ySign = %u\n", pack.flags.xSign, pack.flags.ySign);

	dX=pack.dX;
	dY=pack.dY;

	if (pack.flags.xSign)
		dX=dX-255;

	if (pack.flags.ySign)
		dY=dY-255;

	mouseX+=dX;
	mouseY-=dY;

	mouseX=MAX(mouseX, 0);
	mouseY=MAX(mouseY, 0);

	mouseX=MIN(mouseX, 320);
	mouseY=MIN(mouseY, 200);

	mouse->mouse.x = mouseX;
	mouse->mouse.y = mouseY;
	mouse->mouse.buttons = 0;

//	printf("x = %u, y = %u\n", mouse->mouse.x, mouse->mouse.y);

	if (pack.flags.leftB)
		mouse->mouse.buttons |= MOUSE_LEFTBUTTON;

	if (pack.flags.middleB)
		mouse->mouse.buttons |= MOUSE_MIDDLEBUTTON;

	if (pack.flags.rightB)
		mouse->mouse.buttons |= MOUSE_RIGHTBUTTON;

	return 0;
}

void s_video_null_mouse_uninit (void)
{
}

#endif /* VIDEO_NULL */
