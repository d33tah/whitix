#include "handle.h"
#include "keyset.h"

struct RegKeySetHandle* RegKeySetHandleCreate(char* name, struct RegKeySetHandle** list)
{
	struct RegKeySetHandle* handle;
	struct RegKeySet* set;
	int handleId=0;
	
	set=RegKeySetGet(name);
	
	if (!set)
		return 0;
	
	handle=(struct RegKeySetHandle*)malloc(sizeof(struct RegKeySetHandle));
	
	handle->set=set;
	handle->next=0;
	
	/* Add to list. */
	if (!*list)
	{
		*list=handle;
	}else{
		struct RegKeySetHandle* curr=*list;
		
		while (curr->next)
		{
			curr=curr->next;
			handleId++;
		}
			
		curr->next=handle;
	}
	
	handle->handleId=handleId;
	
	return handle;
}

int RegKeySetGetHandleId(struct RegKeySetHandle* list, struct RegKeySetHandle* handle)
{
	struct RegKeySetHandle* curr=list;
	
	while (curr && curr != handle)
		curr=curr->next;
	
	if (!curr)
		return -1;
	
	return curr->handleId;
}

struct RegKeySetHandle* RegKeySetGetHandle(struct RegKeySetHandle* list, int id)
{
	struct RegKeySetHandle* curr=list;
	
	while (curr && curr->handleId != id)
		curr=curr->next;
		
	return curr;
}
