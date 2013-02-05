#include <stdio.h>
#include <stdlib.h>
#include "xynth_.h"

int main(int argc, char *argv[])
{
	s_window_t* window;

	s_window_init(&window);
	s_window_new(window, WINDOW_MAIN, NULL);

	s_window_set_title(window, "Whitix installer");
	s_window_set_coor(window, 0, 100, 200, 400, 300);

	s_window_show(window);

	s_window_main(window);

	return 0;
}
