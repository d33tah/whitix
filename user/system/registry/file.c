#include "file.h"

#include <file.h>
#include <stdlib.h>

void StrTrimSpaces(char** str)
{
	char* s;

	if (!str || !*str)
		return;

	s=*str;
	
	/* Trim beginning */
	while (isspace(*s))
		s++;
		
	*str=s;
	
	/* Trim end */
	s=*str+strlen(*str)-1;
	while (isspace(*s))
		*s--='\0';
}

int RegParseKeyValue(char* line, char** key, char** value, char** type, char** description)
{
	char* mid=line;
	char* hash;
	
	while (*mid != '=')
		mid++;
	
	*key=line;
	*value=mid+1;
	
	hash=strrchr(line, '#');
	
	if (hash)
	{
		*hash='\0';
		hash++;
		
		while (!isalnum(*hash))
			hash++;
			
//		printf("after hash = %s\n", hash);
		
		if (isalpha(*hash))
		{
			*description=hash;
			
			/* Type is integer at end of string. */
			*type=hash+strlen(hash)-1;
//			printf("type = %s\n", *type);
			
			(*type)[-1]='\0';
//			printf("description = %s\n", *description);
			StrTrimSpaces(description);
		}else{
			printf("Just type. Handle\n");
		}
		
		StrTrimSpaces(type);
	}
	
	*mid='\0';
	
	StrTrimSpaces(key);
	StrTrimSpaces(value);
	
	return 0;
}

int RegLoadFile(char* name, struct RegKeySet* mount)
{
	struct File* file;
	char line[4096];
	char* key, *value, *type, *desc;
	
	file=FileOpen(name, FILE_READ);
	
	if (!file)
		return -1;
		
	/* Parse through the file. */
	while ((FileReadLine(file, line, 4096)) > 0)
	{
		/* Registry directives are surrounded by [ and ] */
		if (line[0] == '[')
		{
			if (line[strlen(line)-1] != ']')
				return -1;
				
			line[strlen(line)-1] = '\0';
				
			RegParseKeyValue(line+1, &key, &value, NULL, NULL);
			
			/* Check against a list of registry directives. */
			if (!strcmp(key, "Mount"))
			{
				if (!mount)
				{
//					printf("RegServer: mounting at %s\n", value);
					RegKeySetCreate(0, value);
					mount=RegKeySetGet(value);
				}
			}
		}else{
			RegParseKeyValue(line, &key, &value, &type, &desc);
//			printf("key = %s, value = %s\n", key, value);
			
			RegKeyCreate(mount, key, value, type, desc);
		}
	}
		
	FileClose(file);
	
	return 0;
}
