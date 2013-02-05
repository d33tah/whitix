#ifndef FILE_H
#define FILE_H

/* Deals with loading files into fruity */
void FileFree();
void NewFile();
int ReadFile(char* fileName);
int SaveFile(char* fileName);
void NewBottomLine();

struct FileLine
{
	int len,lineNo;
	char* data;
	struct FileLine* prev,*next;
};

extern struct FileLine* fileStart,*fileEnd,*editTop,*editBottom,*currLine;
extern int totalLines;

#endif
