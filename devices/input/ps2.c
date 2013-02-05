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

#include <ps2.h>
#include <error.h>
#include <i386/ioports.h>
#include <module.h>

#define PS2_ACK		0xFA

int Ps2Wait()
{
	int i;

	for (i=0; i<0x10000; i++)
		if (!(inb(0x64) & 0x02))
			return 0;

	return -EIO;
}

SYMBOL_EXPORT(Ps2Wait);

static int Ps2SendByte(int byte)
{
	if (Ps2Wait())
		return -EIO;
		
	outb(0x64, byte);
	
	return 0;
}

static int Ps2SendData(int byte)
{
	if (Ps2Wait())
		return -EIO;
		
	outb(0x60, byte);
	
	return 0;
}

int Ps2GetData()
{
	if (Ps2Wait())
		return -EIO;
		
	return inb(0x60);
}

SYMBOL_EXPORT(Ps2GetData);

int Ps2SendCommand(int command, unsigned char* params, int count, int flags)
{
	int i, ret;
	
	if (flags & PS2_CMD_MOUSE)
		if (Ps2SendByte(0xD4))
			return -EIO;
	
	ret=(flags & PS2_CMD_MOUSE) ? Ps2SendData(command) : Ps2SendByte(command);
		
	if (ret)
		return ret;
	
	/* Check if the PS/2 controller acknowledged our command. */
	if ((flags & PS2_CMD_MOUSE) && Ps2GetData() != PS2_ACK)
		return -EIO;	
	
	if (params && (flags & PS2_CMD_INPUT))
	{
		for (i=0; i<count; i++)
		{
			if (flags & PS2_CMD_MOUSE)
				if (Ps2SendByte(0xD4))
					return -EIO;

			if (Ps2SendData(params[i]))
				return -EIO;

			if (Ps2GetData() != PS2_ACK)
				return -EIO;
		}
	}
			
	if (params && (flags & PS2_CMD_OUTPUT))
	{
		for (i=0; i<count; i++)
		{
			ret=Ps2GetData();
			if (ret < 0)
				return ret;
	
			params[i]=(unsigned char)ret;
		}
	}
	
	return 0;
}

SYMBOL_EXPORT(Ps2SendCommand);
