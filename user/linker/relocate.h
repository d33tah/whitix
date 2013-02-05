#ifndef RELOCATE_H
#define RELOCATE_H

#include "elf.h"

int DlRelocate(struct ElfAuxEnt* auxEntries, struct ElfResolve* resolve, char** argv);
extern struct ElfResolve* loadedModules;
extern struct ElfSymResolve* symbolTables;

#endif
