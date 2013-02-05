#include "builtins.h"

#include <console.h>
#include <errno.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <file.h>

struct ColorEnt
{
   char* name;
   int color;
};

struct ColorEnt colors[]=
{
	{"black", 0 },
	{"red", 1},
	{"green", 2},
	{"yellow", 3},
	{"blue", 4},
	{"magenta", 5},
	{"cyan", 6},
	{"white", 7},
};

int BtInPrompt(char* args[])
{
	if(args[1] != NULL)
	{
		strncpy(prompt, args[1], 1);

		if(args[2] != NULL)
		{
			int i = 0;

			for( i = 0; i < 8; i++)
				if(!strcmp(colors[i].name, args[2]))
				{
					promptColor = colors[i].color;
					break;
				}
		}
	}
	
	ConsSetForeColor(promptColor);
	
	return 0;
}

char entryBuf[4096]; /* Put in the BSS section, so it's ok */

#define PATH_MAX 2048
extern char currPath[PATH_MAX];

int BtInChDir(char* args[])
{
	int err;

	if (!args[1])
		printf("cd <directory>\n");
	else{
		err=SysChangeDir(args[1]);

		if (!err)
			SysGetCurrDir(currPath,PATH_MAX);
		else
			printf("cd: %s\n",strerror(-err));
	}

	return 0;
}

int BurnCdTokenUpdate(char* readBuffer, int i, int start, int end)
{
	char* str;
	int ret=CONS_COLOR_RED;
	struct Stat stat;
	
	str=strndup(readBuffer+start, end-start);
	
	str[end-start]='\0';
	
	if (SysStat(str, &stat, -1))
		goto out;
	
	if (!(stat.mode & _SYS_STAT_DIR))
		goto out;
		
	ret=CONS_COLOR_BLUE;

out:
	free(str);
	return ret;
}

/* TODO: Replace with strrchr or console filename? */
static char* FileName(char* str)
{
	int i;
	for (i=strlen(str); i>0; i--)
	{
		if (str[i] == '/')
			return &str[i];
	}

	return str;
}

int BtInCopy(char* args[])
{
	FILE* src,*dest;
	int bytesRead=0;
	char* from=args[1];
	char* to=args[2];
	char* newTo=NULL;

	if (!from || !to)
	{
		printf("copy <from> <to>\nCopies a file from one location to another\n");
		return 0;
	}

	if (from[strlen(from)-1] == '/')
	{
		printf("copy - cannot copy directory\n");
		return 0;
	}

	/* Does to end like a directory? */
	if (to[strlen(to)-1] == '/')
	{
		/* If so, copy over filename of 'from' and srtcat to 'to' */
		char* fileName=FileName(from);
		newTo=malloc(strlen(to)+strlen(fileName)+1);
		if (!newTo)
			return 1;
		strcpy(newTo,to);
		strcat(newTo,fileName);
		to=newTo;
	}

	/* Stat the files in future */
	if (!strcmp(from,to))
	{
		printf("copy: same file\n");
		return 0;
	}

	src=fopen(from,"rb");
	dest=fopen(to,"wb+");

	if (!src)
	{
		printf("copy: Failed to open %s\n",from);
		return 0;
	}

	if (!dest)
	{
		printf("copy: Failed to open %s\n",to);
		return 0;
	}

	while (1)
	{
		bytesRead=fread(entryBuf,1,4096,src);
		if (bytesRead <= 0)
			break;
		fwrite(entryBuf,1,bytesRead,dest);
	}

	fclose(src);
	fclose(dest);

	if (newTo)
		free(newTo);

	return 0;
}

int BtInEcho(char* args[])
{
	if (!args[1])
		printf("\n");
	else{
		args++;

		while (*args)
			printf("%s ",*args++);
		printf("\n");
	}
	
	return 0;
}

int BtInReboot(char* args[])
{
	return SysShutdown(0);
}

int BtInExit(char* args[])
{
	SysExit(0);
	return 0;
}

int BtInView(char* args[])
{
	FILE* file;
	int bytesRead=0;
	int i = 1;

	if (!args[1])
	{
		printf("view <file 1> .. <file n> \n");
		return 0;
	}
	
	while (args[i])
	{	
		file=fopen(args[i],"rb");
		
		if (!file)
		{
			printf("view : failed to open '%s', %s\n",args[i], strerror(errno));
			return 0;
		}

		while (1)
		{
			bytesRead=fread(entryBuf,1,4096,file);
			if (bytesRead <= 0)
				break;
			fwrite(entryBuf,1,bytesRead,stdout);
		}

		printf("\n");
		fclose(file);
	
		i++;
	}

	return 0;
}

int BtInMkDir(char* args[])
{
	int err;

	if (!args[1])
	{
		printf("mkdir <dir name>\nCreate a directory\n");
		return 0;
	}

	err=SysCreateDir(args[1], 0);
	
	if (err < 0)
	{
		errno=-err;
		perror("mkdir");
		errno=0;
	}

	return err;
}

int BtInRmDir(char* args[])
{
	int err;

	if (!args[1])
	{
		printf("rmdir <dir name>\nRemove a directory\n");
		return 0;
	}

	err=SysRemoveDir(args[1]);
	if (err < 0)
	{
		errno=-err;
		perror("rmdir");
		errno=0;
	}

	return 0;
}

int BtInGetCwd(char* args[])
{
	printf("'%s'\n",currPath);
	return 0;
}

int BtInRm(char* args[])
{
	int err;
	int i = 1;
	
	if (!args[i])
	{
		printf("rm <file>\nRemoves a file\n");
		return 0;
	}
	
	while (args[i])
	{
		err=SysRemove(args[i]);
		
		if (err < 0)
		{
			errno=-err;
			perror("rm");
			errno=0;
		}
		
		i++;
	}

	return 0;
}

int BtInMount(char* args[])
{
	int err;

	if (!args[1] || !args[2])
	{
		printf("mount <device name> <file name>\nMounts a filesystem\n");
		return 0;
	}

	err=SysMount(args[1],args[2],args[3],NULL);
	if (err < 0)
	{
		errno=-err;
		perror("mount");
		errno=0;
	}

	return 0;
}

int BtInUMount(char* args[])
{
	int err;

	if (!args[1])
	{
		printf("%s <mount point>\nUnmounts a filesystem\n",args[0]);
		return 0;
	}

	err=SysUnmount(args[1]);
	if (err < 0)
	{
		errno=-err;
		perror(args[0]);
		errno=0;
	}
	return 0;
}

int BtInSync(char* args[])
{
	printf("Syncing the filesystem..");
	SysFileSystemSync();
	printf("Ok\n");
	return 0;
}

int BtInClear(char* args[])
{
	printf("\33[H\33[J");
	return 0;
}

int BtInStat(char* args[])
{
	struct Stat stats;
	int err;

	if (!args[1])
	{
		printf("stat <file>\nDisplays statistics about a file\n");
		return 0;
	}

	err=SysStat(args[1], &stats, -1);

	if (!err)
	{
		printf("%s\n",args[1]);
		printf("Size: %llu bytes, %llu kilobytes, %llu megabytes\n",stats.size,stats.size/1024,stats.size/1024/1024);
		printf("VNode number : %u\n",stats.vNum);
		printf("VNode mode: ");
		if (stats.mode & _SYS_STAT_FILE)
			printf("File ");
		else if (stats.mode & _SYS_STAT_DIR)
			printf("Directory ");

		if (stats.mode & _SYS_STAT_BLOCK)
			printf("Block device");
		else if (stats.mode & _SYS_STAT_CHAR)
			printf("Character device");

		if (stats.mode & _SYS_STAT_READ)
			printf(" Readable");

		if (stats.mode & _SYS_STAT_WRITE)
			printf(" Writeable");

			putchar('\n');
	}else{
		printf("stat: %s\n",strerror(err));
	}

	return 0;
}

/* Search for executable in paths. */
char* searchPaths[]=
{
	"/Applications/",
	""
};

int ExecFind(char* fileName, char* eName)
{
	int i;
	struct Stat stat;
	
	for (i=0; i<(sizeof(searchPaths)/sizeof(searchPaths[0])); i++)
	{
		/* Prepend searchPaths[i] to eName to find executable. */
		strcpy(fileName,searchPaths[i]);
		strcpy(fileName+strlen(searchPaths[i]),eName);
		
		if (!SysStat(fileName,&stat, -1))
			return 0;
	}
	
	return 1;
}

/* Filename of the executable can be as large as the input buffer,
 * but then there won't be any space for arguments. (and paths are
 * limited to 2048 characters anyway). */
#define EXEC_SIZE 32768
char fileName[EXEC_SIZE];

int BtInExec(char* args[])
{
	int comIndex=0,i, outIndex = 0;
	int fds[]={0,1,2};
	int fd;
	char* outFile, *inFile;
	
	outFile=inFile=NULL;

	if (!strcmp(args[0], "exec"))
		comIndex=1;

	/* No command (after 'exec')? */
	if (!args[comIndex])
		return 1;
		
	/* Search for the '>' symbol */
	for (i=comIndex+1; args[i]; i++)
	{
		if (!strcmp(args[i],">"))
		{
			outFile=args[i+1];
			outIndex = i;
		}
		
		if (!strcmp(args[i], "<"))
			inFile=args[i+1];
	}

	/* I/O redirection is required */
	if (outFile)
	{
		/* Either way, it'll still overwrite the stuff at the beginning of the file. Use fopen? */
		fd=SysOpen(outFile, _SYS_FILE_CREATE_OPEN | _SYS_FILE_READ | _SYS_FILE_WRITE, 0);

		if (fd < 0)
		{
			printf("Failed to open %s, %s\n", outFile, strerror(fd));
			return 0;
		}

		SysTruncate(fd, 0);

		fds[0]=fd;
//		fds[2]=fd; /* Have a seperate one for stderr */	
			
		args[outIndex]=NULL;
		args[outIndex + 1]=NULL;
	}

	if (ExecFind(fileName, args[comIndex]))
		return 1;

	int pid=SysCreateProcess(fileName, fds, &args[comIndex+1]);
	if (pid < 0)
	{
//		if (redirOut)
//			SysClose(fd);
		return 1;
	}else
		SysWaitForProcessFinish(pid,NULL);

//	if (redirOut)
//		SysClose(fd);

	return 0;
}

int BtInHistory()
{
	char* ent = ConsGetHistoryEnt();

	while(ent != NULL)
	{
		printf("%s\n", ent);
		free(ent);
		ent = ConsGetHistoryEnt();
	}

	return 0;
}

struct CmdTable btTable[]={
	{"help",BtInHelp},
	{"l", BtInListDir},
	{"ls",BtInListDir},
	{"list",BtInListDir},
	{"cd",BtInChDir},
	{"chdir",BtInChDir},
	{"cp", BtInCopy},
	{"copy",BtInCopy},
	{"echo",BtInEcho},
	{"reboot",BtInReboot},
	{"exit",BtInExit},
	{"quit", BtInExit},
	{"view",BtInView},
	{"mkdir",BtInMkDir},
	{"rmdir",BtInRmDir},
	{"getcwd",BtInGetCwd},
	{"rm",BtInRm},
	{"mount",BtInMount},
	{"umount",BtInUMount},
	{"unmount",BtInUMount},
	{"sync",BtInSync},
	{"clear",BtInClear},
	{"stat",BtInStat},
	{"prompt",BtInPrompt},
	{"history", BtInHistory},
	{"exec",BtInExec},

	/* Marker value used to terminate the list. */
	{NULL,NULL}
};;
