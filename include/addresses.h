#ifndef ADDRESSES_H
#define ADDRESSES_H

#include <config.h>

#define VA_TO_PA(addr) ((unsigned long)addr - KERNEL_ADDRESS_BASE)
#define PA_TO_VA(addr) ((unsigned long)addr + KERNEL_ADDRESS_BASE)

#endif
