#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void DoAssert(char* fileName,int line)
{
        printf("A program failure occured in the source file %s on line %d",
                fileName,line);
                
        exit(1);
}
