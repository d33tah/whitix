#ifndef SERVER_KEY_H
#define SERVER_KEY_H

/* Key types. */
#define REG_KEY_TYPE_STR	0
#define REG_KEY_TYPE_INT	1

struct RegKey
{
	char* key;
	
	int type;
	
	union
	{
		char* sValue;
		int iValue;
	};
	
	char* description;
	
	struct RegKey* next;
};

#endif
