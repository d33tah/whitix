#ifndef _POSIX_TERMIOS_H
#define _POSIX_TERMIOS_H

#include <stdio.h>

typedef int tcflag_t;
typedef int speed_t;
typedef int cc_t;

#define ICANON	0x01
#define IGNBRK	0x02

/* TEMP! */
#define VEOF	0x00
#define VMIN	0x01
#define VTIME	0x02
#define VERASE	0x03
#define VKILL	0x04
#define VSUSP	0x05
#define VINTR	0x06

#define TCIFLUSH	0x01
#define TCSANOW		0x02

#define ECHO	0x01

#define NCCS 6

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
};

struct winsize
{
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

#endif
