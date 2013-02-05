#ifndef VGA_H
#define VGA_H

#include <syscalls.h>

#define outb(port,data) asm volatile("outb %%al,%%dx"::"d"(port),"a"(data))
#define outw(port,data) asm volatile("outw %%ax,%%dx"::"d"(port),"a"(data))

unsigned char inb(unsigned short port) 
{
	unsigned char retVal;
	asm volatile("inb %w1,%b0" : "=a"(retVal) : "d"(port));
	return retVal;
}

#define VGA_BASE		0xA0000
#define VGA_SIZE		0x10000

/* VGA register defines. */
#define VGA_ATT_INDEX		0x3C0
#define VGA_ATT_WRITE		0x3C0
#define VGA_ATT_READ		0x3C1

#define VGA_MISC_READ		0x3CC
#define VGA_MISC_WRITE		0x3C2
#define		VGA_MISC_COLOR_EMUL 	0x01
#define 	VGA_MISC_ENABLE_RAM		0x02
#define		VGA_MISC_SEL_HIGH_BANK	0x20
#define		VGA_MISC_HOR_SYNC_POL	0x40
#define		VGA_MISC_VERT_SYNC_POL	0x80

#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5

#define VGA_PEL_MSK			0x3C6

#define VGA_PAL_INDEX		0x3C8
#define VGA_PAL_DATA		0x3C9

#define VGA_GFX_INDEX		0x3CE
#define VGA_GFX_DATA		0x3CF

#define VGA_CRT_INDEXC		0x3D4
#define VGA_CRT_DATAC		0x3D5


/* VGA sequencer register indices */
#define VGA_SEQ_RESET			0x00
#define		VGA_RESET_1				0x01
#define		VGA_RESET_2				0x02
#define VGA_SEQ_CLOCK_MODE		0x01
#define VGA_SEQ_PLANE_WRITE		0x02
#define VGA_SEQ_CHARACTER_MAP	0x03
#define VGA_SEQ_MEMORY_MODE		0x04
#define VGA_SEQ_NUM				0x5

/* VGA CRT controller register indices */
#define VGA_CRTC_H_TOTAL		0x00
#define VGA_CRTC_H_DISP			0x01
#define VGA_CRTC_H_BLANK_START	0x02
#define VGA_CRTC_H_BLANK_END	0x03
#define VGA_CRTC_H_SYNC_START	0x04
#define VGA_CRTC_H_SYNC_END		0x05
#define VGA_CRTC_V_TOTAL		0x06
#define VGA_CRTC_OVERFLOW		0x07
#define VGA_CRTC_PRESET_ROW		0x08
#define VGA_CRTC_MAX_SCAN       0x09
#define VGA_CRTC_CURSOR_START   0x0A
#define VGA_CRTC_CURSOR_END     0x0B
#define VGA_CRTC_START_HI       0x0C
#define VGA_CRTC_START_LO       0x0D
#define VGA_CRTC_CURSOR_HI      0x0E
#define VGA_CRTC_CURSOR_LO      0x0F
#define VGA_CRTC_V_SYNC_START   0x10
#define VGA_CRTC_V_SYNC_END     0x11
#define VGA_CRTC_V_DISP_END     0x12
#define VGA_CRTC_OFFSET         0x13
#define VGA_CRTC_UNDERLINE      0x14
#define VGA_CRTC_V_BLANK_START  0x15
#define VGA_CRTC_V_BLANK_END    0x16
#define VGA_CRTC_MODE			0x17
#define VGA_CRTC_LINE_COMPARE	0x18
#define VGA_CRTC_NUM			0x19 /* FIXME */

/* Graphics controller register indices */
#define VGA_GFX_SR_VALUE		0x00
#define VGA_GFX_SR_ENABLE		0x01
#define VGA_GFX_COMPARE_VALUE	0x02
#define VGA_GFX_DATA_ROTATE		0x03
#define VGA_GFX_PLANE_READ		0x04
#define VGA_GFX_MODE			0x05
#define VGA_GFX_MISC			0x06
#define VGA_GFX_COMPARE_MASK	0x07
#define VGA_GFX_BIT_MASK		0x08
#define VGA_GFX_NUM				0x09

/* Attribute controller register indices */
#define VGA_ATC_NUM				0x15

#endif
