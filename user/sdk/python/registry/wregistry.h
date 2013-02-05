#ifndef WREGISTRY_H
#define WREGISTRY_H

#include <registry.h>

/* Keyset type. */
typedef struct _WRegKeySet WRegKeySet;
struct wRegKeySet_Type;

struct _WRegKeySet
{
	PyObject_HEAD
	struct RegKeySet keySet;
	PyObject* registry;
};

/* Keyset iterator type. */
typedef struct _WRegKeySetIter WRegKeySetIter;
extern PyTypeObject wRegKeySetIter_Type;

struct _WRegKeySetIter
{
	PyObject_HEAD
	struct RegKeySetIter iter;
	PyObject* keySet;
};

#endif
