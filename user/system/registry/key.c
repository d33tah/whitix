#include "key.h"
#include "keyset.h"

#include <string.h>

int RegStrToType(char* type)
{
	int ret=(*type)-'0';
	
	/* Checks */
	
	return ret;
}

void RegKeyCreate(struct RegKeySet* keySet, char* key, char* value, char* type, char* description)
{
	struct RegKey* new;
	struct RegKeySet* currSet=keySet, *nextSet;
	char* keyName, *path=key;
	int length;
	char* currName=NULL;
	char c;
	
	/* Parse the pathname. */
	currSet=keySet;
	
	while (*path)
	{
		currName=path;
		
		for (length=0; (c=*(path++)) && (c != '/'); length++);
		
		if (!c)
			break;
		
//		printf("currName = %s, length = %d\n", currName, length);
			
		nextSet=RegKeySetLookupOne(currSet, currName, length);
		if (!nextSet)
			nextSet=RegKeySetCreateOne(currSet, currName, length);

		currSet=nextSet;
	}
	
	/* TODO: Does the key already exist? If so, update value */
	key=currName;
	keySet=currSet;
	
//	printf("key = %s\n", key);
	
	new=(struct RegKey*)malloc(sizeof(struct RegKey));
	
	new->key=strdup(key);
	new->next=NULL;
	
	if (type)
		new->type=RegStrToType(type);
	else
		new->type=REG_KEY_TYPE_STR;
		
	switch (new->type)
	{
		case REG_KEY_TYPE_STR:
			new->sValue=strdup(value);
			break;
			
		case REG_KEY_TYPE_INT:
			new->iValue=atoi(value);
			break;
			
		default:
			printf("RegKeyCreate: unknown type %d\n", new->type);
			/* TODO: Recover. */	
	}
	
	if (description)
		new->description=strdup(description);
	
	RegKeySetAddKey(keySet, new);
}
