#include "../../../lib/xynth_.h"
#include "server.h"

#include <syscalls.h>

#define BOCHS_BASE	0xE0000000
#define BOCHS_WIDTH	1024
#define BOCHS_HEIGHT 700
#define BOCHS_BPP	32
#define BOCHS_COLORS	256*65536

#define VBE_DISPI_INDEX		0x01CE
#define VBE_DISPI_DATA		0x01CF

#define VBE_DISPI_XRES		0x1
#define VBE_DISPI_YRES		0x2
#define VBE_DISPI_BPP		0x3 
#define VBE_DISPI_ENABLE	0x4

#define VBE_DISPI_DISABLED	0x00
#define VBE_DISPI_ENABLED	0x01
#define VBE_DISPI_LFB_ENABLED	0x40 

#define outw(port,data) asm volatile("outw %%ax,%%dx"::"d"(port),"a"(data))

extern int noGraphics;

void VbeWrite(unsigned short index, unsigned short value)
{
	outw(VBE_DISPI_INDEX, index);
	outw(VBE_DISPI_DATA, value);
}

void VbeSetMode(unsigned short width, unsigned short height, unsigned short bits)
{
	VbeWrite(VBE_DISPI_ENABLE, VBE_DISPI_DISABLED);
	VbeWrite(VBE_DISPI_XRES, width);
	VbeWrite(VBE_DISPI_YRES, height);
	VbeWrite(VBE_DISPI_BPP, bits);
	VbeWrite(VBE_DISPI_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

int s_video_bochs_server_init (s_server_conf_t *cfg)
{
	SysIoAccess(1);

	if (!noGraphics)
		VbeSetMode(BOCHS_WIDTH, BOCHS_HEIGHT, BOCHS_BPP);

	server->window->surface->width=BOCHS_WIDTH;
	server->window->surface->height=BOCHS_HEIGHT;
	server->window->surface->bytesperpixel=BOCHS_BPP/8;
	server->window->surface->bitsperpixel=BOCHS_BPP;
	server->window->surface->colors = BOCHS_COLORS;
	server->window->surface->blueoffset = 0;
	server->window->surface->greenoffset = 8;
	server->window->surface->redoffset = 16;
	server->window->surface->bluelength = 8;
	server->window->surface->greenlength = 8;
	server->window->surface->redlength = 8;

	int memFd=SysOpen("/System/Devices/Special/Memory", _SYS_FILE_READ | _SYS_FILE_WRITE, 0);

	server->window->surface->linear_mem_size=server->window->surface->width*server->window->surface->height*server->window->surface->bytesperpixel;
	server->window->surface->linear_buf=
		server->window->surface->vbuf=
			(unsigned char*)SysMemoryMap(0, server->window->surface->linear_mem_size, 7, memFd, BOCHS_BASE, _SYS_MMAP_PRIVATE);
	server->window->surface->linear_mem_base=BOCHS_BASE;

	SysClose(memFd);

	if (!server->window->surface->vbuf)
		return -1;

	return 0;
}

void s_video_bochs_server_uninit (void)
{
}

s_video_driver_t s_video_bochs = {
	"bochs",
	"/System/Devices/Special/Memory",
	s_video_bochs_server_init,
	s_video_bochs_server_uninit,
};
