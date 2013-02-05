#ifndef PRINT_H
#define PRINT_H

#include <config.h>

#define KERN_CRITICAL	"<0>"
#define KERN_ERROR		"<1>"
#define KERN_WARNING	"<2>"
#define KERN_INFO		"<3>"
#define KERN_DEBUG		"<4>"

void KePrint(char* message, ...);
void KeSetOutput(void (*newOutput)(char*, int));

#endif
