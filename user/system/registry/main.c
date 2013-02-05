#include <stdio.h>
#include <network.h>
#include <file.h>
#include <syscalls.h>

#include "file.h"
#include "keyset.h"

/*******************************************************************************
 *
 * FUNCTION: 	RegLoadApplication
 *
 * DESCRIPTION: Loads the registry configuration file for a Whitix application 
 *				and mounts it in the registry.
 *
 * PARAMETERS: 	appDir - The path to the directory of the Whitix application.
 *
 * RETURNS: 	Standard Whitix error codes.
 *
 ******************************************************************************/

int RegLoadApplication(char* appDir)
{
	char path[2048];
	struct RegKeySet* mount;
	
	/*
	 * The application's registry file should be located in /Applications/<app>/
	 * application.reg
	 */
	strcpy(path, appDir);
	strcat(path, "/application.reg");
	
	/* Mount the keysets to be read in at /Applications/<app>/, that is, appDir. */
	RegKeySetCreate(NULL, appDir);
	
	mount=RegKeySetGet(appDir);
	
	if (!mount)
		return -1;
	
	/* Load the keysets into memory at the mount point. */
	RegLoadFile(path, mount);
	
	return 0;
}

int RegLoadDirectory(char* name)
{
	struct FileDirectory* dir;
	struct FileDirEnt* entry;
	
	dir=FileDirOpen(name);
	
	if (!dir)
		return -1;
		
	SysChangeDir(name);
		
	while ((entry=FileDirNext(dir)))
	{
		if (entry->name[0] == '.')
			continue;
			
		RegLoadFile(entry->name, NULL);
	}
	
	FileDirClose(dir);
	
	return 0;
}

int main(int argc, char** argv[])
{
	RegKeySetCreateRoot();
	
	RegSocketCreate();
	
	RegKeySetCreate(NULL, "/Applications");
	RegKeySetCreate(NULL, "/System");
	RegKeySetCreate(NULL, "/Users");

	/* Read in the configuration files from the /System/Registry directory. */
	RegLoadDirectory("/System/Registry");
	
	RegLoadApplication("/Applications/Xynth");
	
	while (1)
	{
		RegSocketAcceptConn();
	}
	
	return 0;
}
