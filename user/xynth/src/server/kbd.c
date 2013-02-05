/***************************************************************************
    begin                : Mon Sep 22 2003
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

#include "../lib/xynth_.h"
#include "server.h"
#include "kbd_map.h"
#include "video/helper/kbd.h"

#include <syscalls.h>

extern int noGraphics;
char scanCodeMap[255];
char shiftScanCodeMap[255];

void s_server_kbd_switch_handler (s_window_t *window, s_event_t *event, s_handler_t *handler)
{
        int f = -1;
	s_video_input_t *keybd;
	keybd = (s_video_input_t *) handler->data;

        switch (event->keybd->button) {
		case S_KEYCODE_F1:  f = 1;  break;
		case S_KEYCODE_F2:  f = 2;  break;
		case S_KEYCODE_F3:  f = 3;  break;
		case S_KEYCODE_F4:  f = 4;  break;
		case S_KEYCODE_F5:  f = 5;  break;
		case S_KEYCODE_F6:  f = 6;  break;
		case S_KEYCODE_F7:  f = 7;  break;
		case S_KEYCODE_F8:  f = 8;  break;
		case S_KEYCODE_F9:  f = 9;  break;
		case S_KEYCODE_F10: f = 10; break;
	};

	if (f >= 0) {
#if defined(VIDEO_HELPER_CONSOLE)
		int s_video_helper_console_switch (int);
		s_video_helper_console_switch(f);
#endif
		memset(server->window->event->keybd, 0, sizeof(s_keybd_t));
		server->window->event->type &= ~KEYBD_MASK;
	}
}

void s_server_kbd_window_close_handler (s_window_t *window, s_event_t *event, s_handler_t *handler)
{
	s_server_window_close(window);
}

void s_server_kbd_server_quit_handler (s_window_t *window, s_event_t *event, s_handler_t *handler)
{
	s_server_quit(window);
}

int KbdUpdate(s_video_input_data_t* keybd)
{
	unsigned char scan, key;
	S_KEYCODE_FLAG flag = server->window->event->keybd->flag;

	SysRead(1, &scan, 1);

	keybd->keybd.state = (scan & 0x80) ? KEYBD_RELEASED : KEYBD_PRESSED;

	key=(scan & 0x7F);
	keybd->keybd.scancode=scan;
	
	keybd->keybd.button = s_video_helper_keybd_keycode_[key][KEYCODE_PLAIN];
	keybd->keybd.keycode = s_video_helper_keybd_keycode_[key][KEYCODE_PLAIN];

	switch (keybd->keybd.button)
	{
		case S_KEYCODE_LEFTSHIFT:
		case S_KEYCODE_RIGHTSHIFT:
		case S_KEYCODE_LEFTCONTROL:
		case S_KEYCODE_RIGHTCONTROL:
		case S_KEYCODE_ALT:
		case S_KEYCODE_ALTGR:
		case S_KEYCODE_CAPS_LOCK:
			return 0; /* CHECK */
		default:
			break;
	}
	
	if ((flag & KEYCODE_LSHIFTF) || (flag & KEYCODE_RSHIFTF))
		keybd->keybd.ascii = shiftScanCodeMap[key];
	else
		keybd->keybd.ascii=scanCodeMap[key];

	return 0;
}

int s_server_kbd_update (s_window_t *window, s_pollfd_t *pfd)
{
	int force_release = 0;
	s_video_input_data_t kdata;
	server->window->event->type = 0;
	memset(&kdata, 0, sizeof(s_video_input_data_t));
	
	force_release = KbdUpdate(&kdata);

	s_server_event_parse_keyboard(&(kdata.keybd));
	s_server_event_changed();
	if (force_release) {
		server->window->event->type = 0;
		kdata.keybd.state = KEYBD_RELEASED;
		s_server_event_parse_keyboard(&(kdata.keybd));
		s_server_event_changed();
	}
	
	return 0;
}

int KbdInit()
{
	int flags=5;
	
	/* Get keyboard map from kernel. */
	if (SysConfRead("/Devices/Input/Keyboard/keyMap", scanCodeMap, 255) <= 0)
	{
		printf("Could not read keyboard map. Exiting\n");
		exit(0);
	}
	
	if (SysConfRead("/Devices/Input/Keyboard/shiftKeyMap", shiftScanCodeMap, 255) <= 0)
	{
		printf("Could not read keyboard shift key map. Exiting\n");
		exit(0);
	}
	
	if (!noGraphics)
		flags |= 2;

	if (SysConfWrite("/Devices/Consoles/currConsole/flags", &flags, 4) <= 0)
	{
		printf("Could not set console flags. Exiting\n");
		exit(0);
	}
			
	return 1;
}

void s_server_kbd_init (s_server_conf_t *cfg)
{
	int i;
	int fd = -1;
	s_pollfd_t *pfd;
	s_handler_t *hndl;

    fd=KbdInit();

	if (fd < 0)
		return;

	s_pollfd_init(&pfd);
	pfd->fd = fd;
        pfd->pf_in = s_server_kbd_update;
        pfd->pf_close = s_server_kbd_uninit;
        pfd->data = NULL;
        s_pollfd_add(server->window, pfd);

        s_handler_init(&hndl);
        hndl->type = KEYBD_HANDLER;
	hndl->keybd.flag = KEYCODE_LALTF;
	hndl->keybd.button = S_KEYCODE_F4;
	hndl->keybd.p = s_server_kbd_window_close_handler;
	s_handler_add(server->window, hndl);

        s_handler_init(&hndl);
        hndl->type = KEYBD_HANDLER;
	hndl->keybd.flag = KEYCODE_LCTRLF | KEYCODE_LALTF;
	hndl->keybd.button = S_KEYCODE_DELETE;
	hndl->keybd.p = s_server_kbd_server_quit_handler;
	s_handler_add(server->window, hndl);

	for (i = 1; i <= 10; i++) {
	        s_handler_init(&hndl);
        	hndl->type = KEYBD_HANDLER;
		hndl->keybd.flag = KEYCODE_LSHIFTF;
	        switch (i) {
			case 1:  hndl->keybd.button = S_KEYCODE_F1;  break;
			case 2:  hndl->keybd.button = S_KEYCODE_F2;  break;
			case 3:  hndl->keybd.button = S_KEYCODE_F3;  break;
			case 4:  hndl->keybd.button = S_KEYCODE_F4;  break;
			case 5:  hndl->keybd.button = S_KEYCODE_F5;  break;
			case 6:  hndl->keybd.button = S_KEYCODE_F6;  break;
			case 7:  hndl->keybd.button = S_KEYCODE_F7;  break;
			case 8:  hndl->keybd.button = S_KEYCODE_F8;  break;
			case 9:  hndl->keybd.button = S_KEYCODE_F9;  break;
			case 10: hndl->keybd.button = S_KEYCODE_F10; break;
		};
		hndl->keybd.p = s_server_kbd_switch_handler;
		hndl->data = NULL;
		s_handler_add(server->window, hndl);
	}
}

int s_server_kbd_uninit (s_window_t *window, s_pollfd_t *pfd)
{
	printf("kbd_uninit\n");
	s_server_quit(server->window);
    return 0;
}

S_KEYCODE_CODE s_server_keyname_to_keycode (char *name)
{
	unsigned int i;
	for (i = 0; i < S_KEYCODE_CODES; i++) {
		if (strcmp(name, s_keycodes_map[i].name) == 0) {
			return s_keycodes_map[i].code;
		}
	}
	return 0;
}
