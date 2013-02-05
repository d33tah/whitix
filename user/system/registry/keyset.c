#include "keyset.h"
#include "key.h"

#include <string.h>

struct RegKeySet root;

int RegKeySetCreateRoot()
{
	root.name=strdup("/");
	
	root.children=NULL;
	root.keys=NULL;
	root.numChildren=0;
	root.numKeys=0;
	
	return 0;
}

struct RegKeySet* RegKeySetGetRoot()
{
	return &root;
}

struct RegKeySet* RegKeySetLookupOne(struct RegKeySet* currSet, char* currName, int length)
{
	/* Look through currSet's list of children for currName. */
	struct RegKeySet* currChild=currSet->children;
		
	while (currChild)
	{
		if (!strncasecmp(currChild->name, currName, length))
			break;
				
		currChild=currChild->next;
	}
	
	return currChild;
}

struct RegKeySet* RegKeySetLookup(struct RegKeySet* keySet, char* path, char** name)
{
	struct RegKeySet* currSet, *currChild;
	char* currName=NULL;
	int length=0;
	char c;
	
	if (path[0] == '/')
	{
		currSet=&root;
		path++;
	}else
		currSet=keySet;
		
	if (!currSet)
		return NULL;
	
	while (*path)
	{
		currName=path;
		
		for (length=0; (c=*(path++)) && (c != '/'); length++);
		
		if (!c)
			break;
			
		currSet=RegKeySetLookupOne(currSet, currName, length);
		
		if (!currSet)
			return NULL;
	}
	
	*name=currName;
	
	return currSet;
}

struct RegKey* RegKeySetGetKey(struct RegKeySet* keySet, int id)
{
	struct RegKey* curr=keySet->keys;

    while (curr && id--)
        curr=curr->next;
	
    return curr;
}

struct RegKey* RegKeySetFindKey(struct RegKeySet* keySet, char* name)
{
	struct RegKey* currKey=keySet->keys;
		
	while (currKey)
	{		
		if (!strcasecmp(currKey->key, name))
			break;
				
		currKey=currKey->next;
	}
	
	return currKey;
}

void RegKeySetAddKey(struct RegKeySet* parent, struct RegKey* key)
{
	/* TODO: Check name in keysets. */

	if (!parent->keys)
	{
		parent->keys=key;
	}else{
		struct RegKey* curr=parent->keys;
		
		while (curr->next)
			curr=curr->next;
			
		curr->next=key;
	}
	
	parent->numKeys++;
}

struct RegKeySet* RegKeySetGetChild(struct RegKeySet* list, int id)
{
	struct RegKeySet* curr=list;
	
	while (curr && id--)
		curr=curr->next;
		
	return curr;
}

void RegKeySetAddChild(struct RegKeySet* parent, struct RegKeySet* child)
{
	/* Add the child to the linked list of the parent. */
	if (!parent->children)
	{
		parent->children=child;
	}else{
		struct RegKeySet* curr=parent->children;
		
		while (curr->next)
			curr=curr->next;
			
		curr->next=child;
	}
	
	parent->numChildren++;
}

struct RegKeySet* RegKeySetCreateOne(struct RegKeySet* parent, char* name, int length)
{
	struct RegKeySet* ret;
	
	/* TODO: Check name in keys. */
	
	ret=(struct RegKeySet*)malloc(sizeof(struct RegKeySet));
	ret->name=strndup(name, length);
	ret->children=NULL;
	ret->numChildren=0;
	ret->keys=NULL;
	ret->numKeys=0;
	ret->next=NULL;
	
	RegKeySetAddChild(parent, ret);

	return ret;
}

struct RegKeySet* RegKeySetGet(char* path)
{
	struct RegKeySet* parent;
	char* name;
	
	if (!strcmp(path, "/"))
		return RegKeySetGetRoot();
	
	parent=RegKeySetLookup(NULL, path, &name);
	
	return RegKeySetLookupOne(parent, name, strlen(name));
}

void RegKeySetCreate(struct RegKeySet* keySet, char* path)
{
	struct RegKeySet* parent;
	char* name;
	int len;
	
	/* Parse the path. */
	parent=RegKeySetLookup(keySet, path, &name);
	
	len=strlen(name);
	
	/* Look up the name. If it exists already, don't create it again. */
	if (!RegKeySetLookupOne(parent, name, len))
		RegKeySetCreateOne(parent, name, len);
	
}
