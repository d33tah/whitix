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

#include <error.h>
#include <fs/kfs.h>
#include <string.h>
#include <keobject.h>
#include <malloc.h>
#include <module.h>
#include <panic.h>
#include <print.h>
#include <fs/devfs.h>
#include <fs/icfs.h>

void KeObjectInit(struct KeObject* object, struct KeSet* parent)
{
	object->refs = 1;
	object->dir = NULL;
	object->parent = parent;
}

SYMBOL_EXPORT(KeObjectInit);

void KeObjectDetach(struct KeObject* object)
{
	if (object->dir)
		IcFsRemoveDir(object->dir);
	
	if (object->parent)
		ListRemove(&object->next);
		
	MemFree(object->name);
}

SYMBOL_EXPORT(KeObjectDetach);

void KeObjectFree(struct KeObject* object)
{
	struct KeSet* parent = object->parent;
	
	KeObjectDetach(object);

	if (parent && parent->type && parent->type->release)
		parent->type->release(object);
	else
		KernelPanic("No type to free from\n");
}

SYMBOL_EXPORT(KeObjectFree);

static int KeObjectPopulate(struct KeObject* object)
{
	struct KeObjType* type;
	
	if (!object->parent)
		return 0;
		
	type = object->parent->type;

	if (type && type->attributes)	
		/* Use the attributes field of the type to populate the configuration
		 * directory */
		 IcAddAttributes(object, type->attributes);
	
	return 0;
}

int KeObjectAttach(struct KeObject* object, const char* name, ...)
{
	VaList vaList;
	int err;
	
	VaStart(vaList, name);
	
	err = KeObjectVaAttach(object, name, vaList);
	
	VaEnd(vaList);
	
	return err;
}

int KeObjectVaAttach(struct KeObject* object, const char* name, VaList vaList)
{
	if (!object)
		return -EFAULT;
		
	/* If the pointer to the IcFs entry is already set, we've already bound 
	 * this object to the filesystem.
	 */
	if (object->dir)
		return -EEXIST;

	object->name = vasprintf(name, vaList);
	
	if (!object->name)
		return -EFAULT;
	
	/* Is there already an object in the set with the same name? */
	if (object->parent && KeSetFind(object->parent, object->name))
		return -EEXIST;
		
	/* Create a directory in IcFs, using the object's parent directory as the
	 * parent directory.
	 */
	 
	 object->dir=IcFsCreateDir(KeObjGetParentDir(object), object->name);
	 
	 if (!object->dir)
	 	return -EIO;
	 	
	 /* Populate the configuration directory. */
	 KeObjectPopulate(object);
	 
	 /* Add the new object to the parent's list of KeObjects. */
	 KeSetAdd(object->parent, object);
	 
	 return 0;
}

int KeObjUnbindFs(struct KeObject* object)
{
	/* TODO */
	return 0;
}

struct KeObject* KeSetFind(struct KeSet* set, const char* name)
{
	struct KeObject* curr;
	
	SpinLock(&set->lock);
	
	ListForEachEntry(curr, &set->head, next)
	{
		const char* objName = KeObjGetName(curr);
		
		if (!objName)
			continue;
			
		if (!strcmp(name, objName))
			goto out;
	}
	
	curr = NULL;

out:
	SpinUnlock(&set->lock);
	return curr;
}

void KeSetInit(struct KeSet* set, struct KeSet* parent, struct KeObjType* type)
{
	INIT_LIST_HEAD(&set->head);
	KeObjectInit(&set->object, parent);
	
	set->devDir = NULL;
	set->type = type;
}

SYMBOL_EXPORT(KeSetInit);

int KeSetAttach(struct KeSet* set, const char* name, ...)
{
	if (!set)
		return -EFAULT;
		
	return KeObjectAttach(&set->object, name);
}

SYMBOL_EXPORT(KeSetAttach);
