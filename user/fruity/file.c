#include "file.h"
#include <stdio.h>
#include <stdlib.h>

/* Only a single file supported at the moment */
FILE* file;
int msDos=0; /* Whether or not the file needs CR and LF */
struct FileLine* fileStart,*fileEnd,*editTop,*editBottom,*currLine;
int totalLines;
extern int isModified;

void FileFree()
{
	/* Free all lines */
	struct FileLine* curr=fileStart,*next;
	next=curr->next;

	for (; curr; curr=next)
	{
		next=curr->next;
		free(curr->data);
		free(curr);
	}
}

void NewFile()
{
	struct FileLine* line=(struct FileLine*)malloc(sizeof(struct FileLine));
	line->len=0;
	line->data=(char*)malloc(1);
	line->data[0]='\0';
	line->prev=line->next=NULL;
	editTop=fileStart=line;
}

struct FileLine* FileReadInLine(char* buf,struct FileLine* prev,int len)
{
	/* Handle null characters */
	struct FileLine* line=(struct FileLine*)malloc(sizeof(struct FileLine));
	line->data=(char*)malloc(len+1);

	memcpy(line->data,buf,len);
	line->data[len]='\0';
	line->len=len;

	if (!totalLines)
	{
		line->prev=line->next=NULL;
		line->lineNo=1;
		fileStart=line;
	}else{
		line->prev=prev;
		
		if (line->prev)
		{
			line->prev->next=line; /* Link the previous line's next to this line */
			line->lineNo=prev->lineNo+1;
		}

		line->next=NULL;
	}

	++totalLines;
	return line;
}

#define BUF_SIZE	255

int FileReadIn(FILE* file)
{
	char c=0;
	int len=0;
	struct FileLine* prev=NULL;
	char buf[255];

	if (!file)
		return 1;

	while ((c=fgetc(file)) != EOF)
	{
		/* Doesn't matter about these yet */
		if (c == '\r')
		{
			msDos=1;
			continue;
		}

		if (len == BUF_SIZE)
		{
			printf("Todo: Handle long lines (e.g. binary files)\n");
			return 1;
		}

		if (c == '\n')
		{
			/* Copy the buffer and insert */
			prev=FileReadInLine(buf,prev,len);
			fileEnd=prev;
			len=0;
			buf[0]='\0';
		}else
			buf[len++]=c;
	}

	/* Just in case last line did not end with a newline */
	if (len)
		FileReadInLine(buf,prev,len);
	
	editTop=fileStart;

	return 0;
}

int ReadFile(char* fileName)
{
	int ret=0;

	if (!fileName)
		return -1;

	file=fopen(fileName,"rt");

	if (file)
		ret=FileReadIn(file);
	else
		NewFile();

	isModified=0;

	fclose(file);
	
	return ret;
}

int SaveFile(char* fileName)
{
	/* Write out lines */
	struct FileLine* curr;

	if (!fileName)
		return 1;
	
	file=fopen(fileName,"wt");
	if (!file)
		return 1;
	
	for (curr=fileStart; curr; curr=curr->next)
	{
		/* This line is not written to the file, it's meant for editing at the bottom of a file */
		if (!curr->next && !curr->data[0])
			break;
		
		if (fwrite(curr->data,1,curr->len,file) < curr->len)
			return 1;

		if (msDos)
			fputc('\r',file);
		fputc('\n',file);
	}

	isModified=0;
	
	fclose(file);
	return 0;
}

void NewBottomLine()
{
	fileEnd->next=(struct FileLine*)malloc(sizeof(struct FileLine));
	fileEnd->next->data=(char*)malloc(1);
	fileEnd->next->data[0]='\0';
	fileEnd->next->prev=fileEnd;
	fileEnd->next->next=NULL;
	fileEnd=fileEnd->next;
}
