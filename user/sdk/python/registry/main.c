#include <Python.h>
#include <structmember.h>

/* Registry includes */
#include <registry.h>

#include "wregistry.h"

extern PyTypeObject wRegKeySet_Type;

typedef struct _WRegistry WRegistry;
struct Registry_Type;

struct _WRegistry
{
	PyObject_HEAD
	struct Registry registry;
};

static void wRegistry_dealloc(WRegistry* self)
{
	PyObject_Del(self);
}

static PyObject* wRegistry_repr(WRegistry* self)
{
	printf("wRegistry_repr\n");
	
	return NULL;
}

PyDoc_STRVAR(wRegistry_doc,
"The Registry class abstracts a connection to a Whitix registry.");

/* Registry class methods */
static int wRegistry_Init(PyObject* obj, PyObject *args, PyObject *kwds)
{
	WRegistry* self=(WRegistry*)obj;
	char* name=NULL;
	
	if (!PyArg_ParseTuple(args, "|z:Registry", &name))
		return -1;
		
	if (RegRegistryOpen(name, &self->registry))
		return -1;
		
	return 0;
}

static PyObject* wRegistry_OpenRegistry(PyObject* obj, PyObject* args)
{
	WRegistry* self=(WRegistry*)obj;
	char* name=NULL;
	
	if (!PyArg_ParseTuple(args, "|z:Registry.open", &name))
		return NULL;
		
	if (RegRegistryOpen(name, &self->registry))
		return NULL;
		
	Py_INCREF(Py_None);

	return Py_None;
}

PyDoc_STRVAR(doc_OpenRegistry,
"The open method connects to a Whitix registry.");

static PyObject* wRegistry_OpenKeySet(PyObject* obj, PyObject* args)
{
	WRegistry* self=(WRegistry*)obj;
	WRegKeySet* regKeySet;
	char* name=NULL;
	unsigned long accessRights=0;
	struct RegKeySet keySet;
	
	if (!PyArg_ParseTuple(args, "s|l:Registry.openKeySet", &name, &accessRights))
		return NULL;
	
	if (RegKeySetOpen(&self->registry, name, accessRights, &keySet))
		return PyErr_Format(PyExc_KeyError, "Could not find key %s", name);
			
	regKeySet=PyObject_GC_New(WRegKeySet, &wRegKeySet_Type);
	
	if (regKeySet == NULL)
		return NULL;
	
	memcpy(&regKeySet->keySet, &keySet, sizeof(struct RegKeySet));
	
	regKeySet->registry=(PyObject*)self;

	Py_INCREF(self);
#if 0
	PyObject_GC_Track(regKeySet);
	
	return (PyObject*)regKeySet;
#endif

	return (PyObject*)regKeySet;
}

PyDoc_STRVAR(doc_OpenKeySet,
"The openKeySet method opens for a keyset for reading, enumerating or updating.");

static PyMethodDef wRegistry_methods[] = {
	{"open", wRegistry_OpenRegistry, METH_VARARGS, doc_OpenRegistry},
	{"openKeySet", wRegistry_OpenKeySet, METH_VARARGS, doc_OpenKeySet},
//	{"closeKeySet", wRegistry_CloseKeySet, METH_VARARGS, doc_CloseKeySet},
	{NULL, NULL, 0, NULL},
};

static PyTypeObject wRegistry_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"wregistry.Registry",
	sizeof(WRegistry),
	0,					/* tp_itemsize */
	(destructor)wRegistry_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)wRegistry_repr,		/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	wRegistry_doc,			/* tp_doc */
	0,			/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	wRegistry_methods,			/* tp_methods */
	/* wRegistry_members */ 0,			/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	wRegistry_Init,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	PyObject_GC_Del,			/* tp_free */
};

PyDoc_STRVAR(docModule,
"Importing this module enables usage of the Whitix registry API.");

PyMODINIT_FUNC initwregistry(void)
{
	Py_Initialize();

	PyObject* m;
	
	wRegistry_Type.ob_type=&PyType_Type;
	
	if (PyType_Ready(&wRegistry_Type) < 0)
		return;

	wRegKeySet_Type.ob_type=&PyType_Type;
	
	if (PyType_Ready(&wRegKeySet_Type) < 0)
		return;
		
	wRegKeySetIter_Type.ob_type=&PyType_Type;
	
	if (PyType_Ready(&wRegKeySetIter_Type) < 0)
		return;

	m = Py_InitModule4("wregistry", NULL, docModule,
			   (PyObject *)NULL, PYTHON_API_VERSION);

	if (m == NULL)
		return;
		
	Py_INCREF(&wRegistry_Type);
	PyModule_AddObject(m, "Registry",
			       (PyObject *)&wRegistry_Type);
		
	if (PyModule_AddObject(m, "RegKeySet",
					(PyObject *)&wRegKeySet_Type) < 0)
		return;
		
	if (PyModule_AddObject(m, "RegKeySetIter",
					(PyObject *)&wRegKeySetIter_Type) < 0)
		return;
		
	PyModule_AddIntConstant(m, "EntryKey", 0);
	PyModule_AddIntConstant(m, "EntryKeySet", 1);
}
