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

#include <Python.h>
#include <structmember.h>
#include "xynth.h"
#include "../widget/widget.h"

/* event.c */
void p_event_dealloc (PyObject *obj);
PyObject * p_event_new (PyTypeObject *type, PyObject *args, PyObject *kwds);
PyObject * wrap_event (PyObject *self, PyObject *args);

/* xynthpy.c */
void initxynth (void);

typedef struct p_window_s {
	PyObject_HEAD
	s_window_t *window;
	PyObject *atevent;
	PyObject *atexit;
} p_window_t;

PyTypeObject p_window_type;
