#include <Python.h>
#include <structmember.h>

/* Registry includes */
#include <registry.h>

#include "wregistry.h"

static void wRegKeySetIter_dealloc(WRegKeySetIter* self)
{
	RegKeySetIterClose(&self->iter);
	
	Py_XDECREF(self->keySet);
	PyObject_GC_Del(self);
}

static PyObject* wRegKeySetIter_repr(WRegKeySetIter* self)
{
	printf("wRegKeySetIter_repr\n");
	return self;
}

static PyObject* wRegKeySetIter_iter(WRegKeySetIter* self)
{
//	printf("self->index = %d\n", self->index);
	
	return self;
}

PyDoc_STRVAR(wRegKeySetIter_doc,
"A Python iterator for enumerating child keysets.");

static PyObject* wRegKeySetIter_Next(WRegKeySetIter* self)
{
	/* Create a string object. */
	char* name;
	PyObject* str;
	
	name=RegKeySetIterNext(&self->iter);
	
	if (name)
		str=PyString_FromString(name);
	else
	{
		PyErr_SetNone(PyExc_StopIteration);
		str=NULL;
	}
		
	return str;
}

PyTypeObject wRegKeySetIter_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"wregistry.RegKeySetIter",
	sizeof(WRegKeySetIter),
	0,					/* tp_itemsize */
	(destructor)wRegKeySetIter_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)wRegKeySetIter_repr,		/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
		Py_TPFLAGS_HAVE_GC,		/* tp_flags */
	wRegKeySetIter_doc,			/* tp_doc */
	0,			/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	wRegKeySetIter_iter,					/* tp_iter */
	wRegKeySetIter_Next,					/* tp_iternext */
	0,			/* tp_methods */
	/* wRegKeySet_members */ 0,			/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	PyObject_GC_Del,			/* tp_free */
};
