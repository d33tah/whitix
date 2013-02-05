/***************************************************************************
    begin                : Sat Jul 2 2005
    copyright            : (C) 2005 - 2007 by Alper Akcan
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

#include "xynthpy.h"

extern PyTypeObject p_frame_type;
extern PyTypeObject p_window_type;

PyMethodDef xynth_methods[] = {
//	{"window", wrap_window, 1},
//	{"event", wrap_event, 1},
	{NULL, NULL}
};

PyDoc_STRVAR(docModule,
"Importing this module enables usage of the Xynth windowing system.");

PyMODINIT_FUNC initxynth (void)
{
	PyObject *m;

	p_frame_type.ob_type = &PyType_Type;
	p_window_type.ob_type = &PyType_Type;

	if (PyType_Ready(&p_window_type) < 0) {
		return;
	}
	
	if (PyType_Ready(&p_frame_type) < 0)
		return;
	
	m = Py_InitModule4("xynth", NULL, docModule, (PyObject*)NULL, PYTHON_API_VERSION);
	
	if (m == NULL)
		return;
		
	Py_INCREF(&p_window_type);
	
	if (PyModule_AddObject(m, "Window", (PyObject *)&p_window_type))
		return;
	
	Py_INCREF(&p_frame_type);
		
	if (PyModule_AddObject(m, "Frame", (PyObject *)&p_frame_type))
		return;
	
	PyModule_AddIntConstant(m, "WINDOW_NOFORM", WINDOW_NOFORM);
	PyModule_AddIntConstant(m, "WINDOW_MAIN", WINDOW_MAIN);

	PyModule_AddIntConstant(m, "FRAME_MENUBARPANEL", FRAME_MENUBARPANEL);
	
//	PyModule_AddObject(m, "event", (PyObject *) ((void *) (&p_event_type)));
}
