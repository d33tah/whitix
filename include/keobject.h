/*****************************************************************************
 * 									     									 *
 * kobject.h - Kernel object infrastructure				     				 *
 * 									     									 *
 ****************************************************************************/

/*  This file is part of Whitix.
 *
 *  Whitix is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Whitix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Whitix; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef KEOBJECT_H
#define KEOBJECT_H

#include <llist.h>
#include <fs/icfs.h>
#include <fs/kfs.h>
#include <spinlock.h>
#include <typedefs.h>
#include <stdarg.h>

struct IcObject;
struct KeObject;
struct KeSet;

struct KeObjType
{
	void (*release)(struct KeObject* object);
	struct IcAttribute* attributes;
	int offset; /* Offset of the KeObject structure in the type. */
};

#define KE_OBJECT_TYPE(name, releaseFunc, objAttributes, objOffset) \
	struct KeObjType name = \
	{ \
		.release = releaseFunc, \
		.attributes = objAttributes, \
		.offset = objOffset, \
	}
	
#define KE_OBJECT_TYPE_SIMPLE(name, releaseFunc) \
	KE_OBJECT_TYPE(name, releaseFunc, NULL, 0)

struct KeObject
{
	/* The object's name; this can be null. */
	char* name;
	
	int refs;
	
	/* The parent set. Every KeObject has a parent set, since they link to the
	 * object's type. Even KeObjects that are not linked to the filesystem must
	 * have a parent KeSet.
	 */
	struct KeSet* parent;
	
	/* dir is the directory containing all the object attributes in icfs */
	struct KeFsEntry* dir;
	
	/* Used in the parent KeSet list */
	struct ListHead next;
};

struct KeSet
{
	struct ListHead head;
	Spinlock spinLock;
	struct KeObject object;
	struct KeFsEntry* devDir; /* TODO: Create bus and class structures, use those instead. */
	struct KeObjType* type;
};

/* Prototypes */
void KeObjectInit(struct KeObject* object, struct KeSet* parent);
void KeObjectFree(struct KeObject* object);

int KeObjectAttach(struct KeObject* object, const char* name, ...);
int KeObjectVaAttach(struct KeObject* object, const char* name, VaList vaList);
int KeObjectRemove(struct KeObject* object);

static inline int KeObjectCreate(struct KeObject* object, struct KeSet* parent, const char* name, ...)
{
	VaList vaList;
	int err;
	
	KeObjectInit(object, parent);
	
	VaStart(vaList, name);
	
	err = KeObjectVaAttach(object, name, vaList);
	
	VaEnd(vaList);
	
	return err;
}

/* All tests can probably be debug code */

/* Get the type most specific to the object. */
static inline struct KeObjType* KeObjectGetType(struct KeObject* object)
{
	if (!object)
		return NULL;
		
	return object->parent->type;
}

static inline void KeObjectSetType(struct KeObject* object, struct KeObjType* type)
{
}

static inline int KeObjGet(struct KeObject* object)
{
	if (!object)
		return NULL;
		
	object->refs++;
	
	return object->refs;
}

static inline int KeObjPut(struct KeObject* object)
{
	if (!object)
		return NULL;
		
	object->refs--;
	
	if (object->refs == 0)
	{
		KeObjectFree(object);
		return 0;
	}
	
	return object->refs;
}

static inline const char* KeObjGetName(struct KeObject* object)
{
	if (!object)
		return NULL;
		
	if (object->name)
		return object->name;
		
	if (!object->dir)
		return NULL;
		
	return object->dir->name;
}

static inline struct KeFsEntry* KeObjGetParentDir(struct KeObject* object)
{
	if (!object)
		return NULL;
		
	if (!object->parent)
		return NULL;
		
	return object->parent->object.dir;
}

/* KeObjGetParentDir
 * -----------------
 *
 * Since every KeObject has a KeSet as a parent, we can use this fact to get
 * the DevFs directory that the KeObject should go in easily.
 *
 * TODO: Like elsewhere, create bus and class hierarchies.
 */

static inline struct KeFsDir* KeObjGetParentDevDir(struct KeObject* object)
{
	if (!object || !object->parent || !object->parent->devDir)
		return NULL;
		
	return &object->parent->devDir->dir;
}

static inline struct KeSet* KeObjGetSet(struct KeObject* object)
{
	return object->parent;
}

/* KeSet API */
void KeSetInit(struct KeSet* set, struct KeSet* parent, struct KeObjType* type);
int KeSetAttach(struct KeSet* set, const char* name, ...);

static inline int KeSetCreate(struct KeSet* set, struct KeSet* parent,
	 struct KeObjType* type, const char* name)
{
	KeSetInit(set, parent, type);
		
	return KeSetAttach(set, name);
}

/* KeSetAdd
 *
 * Add the KeObject object to the KeSet set.
 */
 
static inline void KeSetAdd(struct KeSet* set, struct KeObject* object)
{
	if (!set)
		return;
	
	KeObjGet(&set->object);
		
	SpinLock(&set->spinLock);
	ListAddTail(&object->next, &set->head);
	SpinUnlock(&set->spinLock);
}

struct KeObject* KeSetFind(struct KeSet* set, const char* name);

struct KeSubsystem
{
	struct KeObject object;
	struct ListHead subSystems;
};

#endif
