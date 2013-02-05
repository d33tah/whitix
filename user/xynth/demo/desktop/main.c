/***************************************************************************
    begin                : Tue Oct 5 2004
    copyright            : (C) 2004 - 2007 by Alper Akcan
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

#include "desktop.h"

int desktop_self_system (char *command)
{
	int i = 0;
	char *tmp;
	char *ptr;
	char *str;
        char *arg[10];
	
	memset(arg, 0, sizeof(char *) * 10);
	
	tmp = strdup(command);
	ptr = tmp;
	str = tmp;
	while (*ptr) {
		arg[i++] = ptr;
		if ((str = strchr(ptr, ' ')) != NULL) {
			*str = '\0';
			ptr = str + 1;
		} else {
			break;
		}
	}

	int fds[]={0, 1, 2};

	char buf[255];
	strcpy(buf, "/Applications/");
	strcat(buf, command);

	SysCreateProcess(buf, fds, arg);

	s_free(tmp);
	return 0;
}

int main (int argc, char *argv[])
{
	s_config_t *cfg;
	s_window_t *desktop;
	s_window_t *taskbar;

	s_config_init(&cfg);
	s_config_parse(cfg, DESKTOPDIR "/cfg/desktop.cfg");

	s_window_init(&desktop);
	s_window_init(&taskbar);

	s_window_new(desktop, WINDOW_MAIN | WINDOW_NOFORM, NULL);
	s_window_new(taskbar, WINDOW_CHILD | WINDOW_NOFORM | WINDOW_DESKTOP, desktop);

	taskbar_start(taskbar, cfg);
	desktop_start(desktop, cfg);

	s_config_uninit(cfg);

	return 0;
}
