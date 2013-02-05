#include <Python.h>
#include <structmember.h>

#include "wregistry.h"

/* Registry includes */
#include <registry.h>

static void wRegKeySet_dealloc(WRegKeySet* self)
{
	Py_XDECREF(self->registry);
	PyObject_GC_Del(self);
}

static PyObject* wRegKeySet_repr(WRegKeySet* self)
{
	printf("wRegKeySet_repr\n");
	return self;
}

PyDoc_STRVAR(wRegKeySet_doc,
"The RegKeySet class abstracts a keyset in the Whitix registry.");

/* Class methods. */

static PyObject* wRegKeySet_ReadString(PyObject* obj, PyObject* args)
{
	char* name=NULL;
	WRegKeySet* self=(WRegKeySet*)obj;
	char* buf;
	unsigned long length=4096;
	PyObject* str;
	
	buf=(char*)malloc(4096);
	
	if (!PyArg_ParseTuple(args, "s:RegKeySet.readString", &name))
		return NULL;
		
	if (RegKeyReadString(&self->keySet, name, &buf, &length))
		return NULL;
		
//	printf("buf = %s, length = %d\n", buf, length);
		
	/* Create a string from buf and return. */
	str=PyString_FromStringAndSize(buf, length);
	
	free(buf);
	
	return str;
}

PyDoc_STRVAR(doc_ReadString,
"The readString method reads the value of the key denoted by the given name.");

static PyObject* wRegKeySet_ReadInt(PyObject* obj, PyObject* args)
{
	char* name=NULL;
	WRegKeySet* self=(WRegKeySet*)obj;
	int ret;
	
	if (!PyArg_ParseTuple(args, "s:RegKeySet.readInt", &name))
		return NULL;
		
	if (RegKeyReadInt(&self->keySet, name, &ret))
		return NULL;
		
	return PyLong_FromLong(ret);
}

PyDoc_STRVAR(doc_ReadInt,
"The readInt method reads the value of the key denoted by the given name.");

static PyObject* wRegistry_GetSetIter(PyObject* obj, PyObject* args)
{
	WRegKeySet* self=(WRegKeySet*)obj;
	WRegKeySetIter* keySetIter;
	
	if (!PyArg_ParseTuple(args, ":Registry.getSetIter"))
		return NULL;
		
	/* Create a RegKeySetIter object and return. */
	keySetIter=PyObject_GC_New(WRegKeySetIter, &wRegKeySetIter_Type);
	RegKeySetIterInit(&self->keySet, &keySetIter->iter);
	
	Py_INCREF(self);
	keySetIter->keySet=self;
	
	return (PyObject*)keySetIter;
}

PyDoc_STRVAR(doc_GetSetIter,
"Create an iterator (RegKeySetIter) for enumerating child sets");

static PyObject* wRegKeySet_GetEntryType(PyObject* obj, PyObject* args)
{
	WRegKeySet* self=(WRegKeySet*)obj;
	char* name=NULL;
	long ret;
	
	if (!PyArg_ParseTuple(args, "s:RegKeySet.getEntryType", &name))
		return NULL;
		
	if (self->registry == NULL)
		return NULL;
		
	if (RegEntryGetType(&self->keySet, name, &ret))
		return NULL;
		
	return PyLong_FromLong(ret);
}

PyDoc_STRVAR(doc_GetEntryType,
"Get the type (either key or keyset) of an entry in the given keyset.");

static PyObject* wRegistry_GetKeyStatistics(PyObject* object, PyObject* args)
{
}

static PyMethodDef wRegKeySet_methods[] = {
	{"readString", wRegKeySet_ReadString, METH_VARARGS, doc_ReadString},
	{"readInt", wRegKeySet_ReadInt, METH_VARARGS, doc_ReadInt},
	{"getSetIter", wRegistry_GetSetIter, METH_VARARGS, doc_GetSetIter},
	{"getEntryType", wRegKeySet_GetEntryType, METH_VARARGS, doc_GetEntryType},
	{NULL, NULL, 0, NULL},
};

PyTypeObject wRegKeySet_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"wregistry.RegKeySet",
	sizeof(WRegKeySet),
	0,					/* tp_itemsize */
	(destructor)wRegKeySet_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)wRegKeySet_repr,		/* tp_repr */
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
	wRegKeySet_doc,			/* tp_doc */
	0,			/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	wRegKeySet_methods,			/* tp_methods */
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
