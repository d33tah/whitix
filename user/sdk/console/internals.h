#ifndef CONSOLE_INTERNALS_H
#define CONSOLE_INTERNALS_H

/* Configurables. */
#define BUFFER_SIZE		32768
#define TABS_TOTAL		100

void ConsDoTabComplete(struct ConsReadContext* context, int i, char* prompt, int promptColor);
void ConsBufferUpdate(struct ConsReadContext* context, int i, int a);
void ConsFilenameOutput(char* str);
void ConsTokenGet(char* string, int i, int* start, int* end);
void ConsTokenGetStr(char* string, int i, char** start, char** end);

int HistoryDisplay(struct ConsReadContext* context, char* prompt, int dir);

#endif
