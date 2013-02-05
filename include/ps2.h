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

#ifndef PS2_H
#define PS2_H

int Ps2SendCommand(int cmd, unsigned char* params, int count, int isMouse);
int Ps2Wait();
int Ps2GetData();

#define Ps2SendSimpleCmd(cmd) (Ps2SendCommand((cmd), NULL, 0, 0))

/* Flags */
#define PS2_CMD_MOUSE		0x1
#define PS2_CMD_INPUT		0x2
#define PS2_CMD_OUTPUT		0x4

/* Generic commands */
#define PS2_CMD_WRITECMD	0x60

#endif
