#ifndef HASH_H
#define HASH_H

unsigned long DlElfHash(char* symName);
unsigned long DlFindHash(char* symName, int type);
void DlAddHashTable(struct ElfResolve* entry);

#endif
