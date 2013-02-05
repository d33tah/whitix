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

#if defined(VIDEO_VGA)

#include "../../../lib/xynth_.h"
#include "server.h"
#include "vga.h"

#include <syscalls.h>

extern int mouseX, mouseY;
extern int noGraphics;

void s_video_vga_server_uninit (void)
{
}

void VgaWriteSeq(unsigned char reg, unsigned char val)
{
	outb(VGA_SEQ_INDEX, reg);
	outb(VGA_SEQ_DATA, val);
}

unsigned char VgaReadSeq(unsigned char reg)
{
	outb(VGA_SEQ_INDEX, reg);
	return inb(VGA_SEQ_DATA);
}

void VgaWriteCrt(unsigned char reg, unsigned char val)
{
	outb(VGA_CRT_INDEXC, reg);
	outb(VGA_CRT_DATAC, val);
}

unsigned char VgaReadCrt(unsigned char reg)
{
	outb(VGA_CRT_INDEXC, reg);
	return inb(VGA_CRT_DATAC);
}

void VgaWriteGfx(unsigned char reg, unsigned char val)
{
	outb(VGA_GFX_INDEX, reg);
	outb(VGA_GFX_DATA, val);
}

void VgaWriteAttr(unsigned char reg, unsigned char val)
{
	outb(VGA_ATT_INDEX, reg);
	outb(VGA_ATT_WRITE, val);
}

unsigned char seqRegisters[VGA_SEQ_NUM]={
	0x03, 0x01, 0x0F, 0x00, 0x0E,
};

unsigned char crtcRegisters[VGA_CRTC_NUM]={
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
	0xFF,
};

unsigned char gfxRegisters[VGA_GFX_NUM]={
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF
};

unsigned char attribRegisters[0x15]={
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00, 0x00,
};

static int VgaInit()
{
	int i;

	SysIoCtl(0, 1, 3);

	VgaWriteSeq(0x01, VgaReadSeq(0x01) | 0x20);
	inb(0x3DA);
	SysYield();
	outb(VGA_ATT_WRITE, 0x00);
	SysYield();

//	outb(VGA_MISC_WRITE, inb(VGA_MISC_READ) | VGA_MISC_COLOR_EMUL);
	outb(VGA_MISC_WRITE,  VGA_MISC_HOR_SYNC_POL 
			| VGA_MISC_SEL_HIGH_BANK | VGA_MISC_ENABLE_RAM | VGA_MISC_COLOR_EMUL);

	/* Synchronous reset on. */
	VgaWriteSeq(VGA_SEQ_RESET, VGA_RESET_1);

//	/* Write sequencer registers. */
//	VgaWriteSeq(VGA_SEQ_CLOCK_MODE, seqRegisters[VGA_SEQ_CLOCK_MODE] | 0x20);

	for (i=1; i<VGA_SEQ_NUM; i++)
		VgaWriteSeq(i, seqRegisters[i]);

	/* Synchronous reset off. */
	VgaWriteSeq(VGA_SEQ_RESET, VGA_RESET_1 | VGA_RESET_2);

	/* Deprotect CRT registers 0-7 */
	VgaWriteCrt(VGA_CRTC_V_SYNC_END, VgaReadCrt(VGA_CRTC_V_SYNC_END) & 0x7F);

	for (i=0; i<VGA_CRTC_NUM; i++)
		VgaWriteCrt(i, crtcRegisters[i]);

	for (i=0; i<VGA_GFX_NUM; i++)
		VgaWriteGfx(i, gfxRegisters[i]);

	for (i=0; i<0x15; i++)
	{
		inb(0x3DA);
		VgaWriteAttr(i, attribRegisters[i]);
		SysYield();
	}

	outb(VGA_PEL_MSK, 0xFF);
	outb(VGA_PAL_INDEX, 0);

	int r, g, b;

	for (i=0; i<256; i++)
	{
		b=(i & 7)*9;
		g=((i >> 3) & 7)*9;
		r=((i >> 6) & 3)*21;

		outb(VGA_PAL_DATA, r);
		outb(VGA_PAL_DATA, g);
		outb(VGA_PAL_DATA, b);
	}

	inb(0x3DA);
	SysYield(); /* FIXME: Wait properly! */

	VgaWriteSeq(VGA_SEQ_CLOCK_MODE, VgaReadSeq(VGA_SEQ_CLOCK_MODE) & 0xDF);

	inb(0x3DA);
	SysYield();
	outb(VGA_ATT_WRITE, 0x20);
	SysYield();

	inb(0x3DA);

	return 0;
}

int s_video_vga_server_init (s_server_conf_t *cfg)
{
	printf("Loading VGA driver.\n");
	server->window->surface->width=320;
	server->window->surface->height=200;
	server->window->surface->bytesperpixel=1;
	server->window->surface->bitsperpixel=8;
	server->window->surface->colors = 256;
	server->window->surface->blueoffset = 0;
	server->window->surface->greenoffset = 3;
	server->window->surface->redoffset = 6;
	server->window->surface->bluelength = 3;
	server->window->surface->greenlength = 3;
	server->window->surface->redlength = 2;

	SysIoAccess(1);

	int memFd=SysOpen("/System/Devices/Special/Memory", _SYS_FILE_READ | _SYS_FILE_WRITE, 0);

	if (memFd < 0)
		printf("Could not open memory device\n");

	server->window->surface->linear_buf=
		server->window->surface->vbuf=
			(unsigned char*)SysMemoryMap(0, VGA_SIZE, 7, memFd, VGA_BASE, _SYS_MMAP_PRIVATE);

	SysClose(memFd);

	if (!server->window->surface->vbuf)
		return -1;

	server->window->surface->linear_mem_base=0xA0000;
	server->window->surface->linear_mem_size=VGA_SIZE;

	if (!noGraphics)
		VgaInit();

	return 0;
}

s_video_driver_t s_video_vga = {
	"vga",
	"/System/Devices/Special/Memory",
	s_video_vga_server_init,
	s_video_vga_server_uninit,
};

#endif /* VIDEO_VGA */
