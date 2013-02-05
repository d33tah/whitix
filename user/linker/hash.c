#include "lib.h"
#include "elf.h"
#include "hash.h"
#include "relocate.h"

unsigned long DlElfGnuHash(char* symName)
{
	unsigned long hash = 5381;
	unsigned char c;
	
	for (c = *symName; c != '\0'; c = *++symName)
		hash = hash * 33 + c;
		
	return hash;
}

unsigned long DlElfHash(char* symName)
{
	unsigned long hash=0,tmp;

	while (*symName)
	{
		hash=(hash << 4) + *symName++;
		tmp=hash & 0xF0000000;
		hash^=tmp;
		hash^=tmp >> 24;
	}

	return hash;
}

unsigned long DlFindHashResolve(char* symName, struct ElfResolve* resolve,
	unsigned long symHash, unsigned long* oldHash, int symType)
{
	struct ElfSym* symTab=(struct ElfSym*)resolve->dynamicInfo[DT_SYMTAB];
	char* strTab=(char*)resolve->dynamicInfo[DT_STRTAB];
	int si;
	unsigned long addr;
	
	unsigned long DlHashCheckMatch(struct ElfSym* sym)
	{
		if (sym->shIndex == SHN_UNDEF)
			return 0;

		if (ELF_ST_TYPE(sym->info) > STT_FUNC &&
			ELF_ST_TYPE(sym->info) != STT_COMMON &&
			ELF_ST_TYPE(sym->info) != STT_TLS)
			return 0;

		if (ELF_ST_TYPE(sym->info) == STT_TLS)
			return DlTlsGetSymAddr(resolve, sym);
		else if (!sym->value)
			return 0;

		if (link_strcmp(symName, strTab+sym->name))
			return 0;

		switch (ELF_ST_BIND(sym->info))
		{
		case STB_WEAK:
		case STB_GLOBAL:
			return sym->value+resolve->loadAddr;
		default:
			return 0;
		}
	}

	/* Avoid searching the main executable when resolving a COPY relocation. */
	if ((symType & ELF_RTYPE_CLASS_COPY) && resolve->type == ELF_EXECUTABLE)
		return 0;
		
	/* If there are no buckets, there's nothing to do here. */
	if (resolve->numBuckets == 0)
		return 0;
		
	if (resolve->gnuBitMask)
	{
		unsigned long* bitMask = (unsigned long*)resolve->gnuBitMask;
		unsigned long bWord, hashBit1, hashBit2;
		
		bWord = bitMask[(symHash/_ELF_NATIVE_CLASS) & resolve->gnuMaskIdxBits];
		
		hashBit1 = symHash & (_ELF_NATIVE_CLASS - 1);
		hashBit2 = (symHash >> resolve->gnuShift) & (_ELF_NATIVE_CLASS - 1);
		
		if ((bWord >> hashBit1) & (bWord >> hashBit2) & 1)
		{
			unsigned long bucket = resolve->gnuBuckets[symHash % resolve->numBuckets];

			if (bucket)
			{
				unsigned long* hashArr = &resolve->gnuChainZero[bucket];
				
				do
				{
					if (((*hashArr ^ symHash) >> 1) == 0)
					{
						si = hashArr - resolve->gnuChainZero;
						addr = DlHashCheckMatch(&symTab[si]);
						
						if (addr)
							return addr;
					}
				} while ((*hashArr++ & 1) == 0);
			}
		}
	}else{	
		if (*oldHash == 0xFFFFFFFF)
			*oldHash = DlElfHash(symName);
	
		symHash = *oldHash;
	
		for (si=resolve->elfBuckets[symHash % resolve->numBuckets]; si!=STN_UNDEF;
			si=resolve->elfChains[si])
		{
			addr = DlHashCheckMatch(&symTab[si]);
						
			if (addr)
				return addr;
		}
	}

	return 0;
}

unsigned long DlFindHash(char* symName, int symType)
{
	unsigned long symHash=DlElfGnuHash(symName);
	unsigned long addr=0, oldHash = 0xFFFFFFFF;
	struct ElfSymResolve* elfSymResolves;

	for (elfSymResolves = symbolTables; elfSymResolves;
		elfSymResolves=elfSymResolves->next)
	{
		if ((addr=DlFindHashResolve(symName, elfSymResolves->resolveEntry, symHash, &oldHash, symType)))
			return addr;
	}

	return 0;
}

void DlAddHashTable(struct ElfResolve* entry)
{
	struct ElfResolve* curr;
	ElfSymIndex* hashAddr;

	if (!loadedModules)
	{
		/* Allocate the loadedModules head. */
		loadedModules=entry;
	}else{
		curr = loadedModules;
		while (curr->next)
			curr = curr->next;

		curr->next = entry;
		entry->prev = curr;
	}

	entry->next=NULL;
	entry->refs=1;
	entry->initFlags=0;

	/* In the hashing code, we prefer to use GNU_HASH over HASH. */

	if (entry->dynamicInfo[DT_GNU_HASH_IDX])
	{
		unsigned long* hash = (unsigned long*)entry->dynamicInfo[DT_GNU_HASH_IDX];
		unsigned long symBias, bitMaskWords;
				
		entry->numBuckets = *hash++;
		
		symBias = *hash++;
		bitMaskWords = *hash++;
		
		if (bitMaskWords & (bitMaskWords-1))
		{
			link_puts(entry->libName);
			link_puts(": bitMaskWords not a power of two. ");
			SysExit(0);
		}
		
		entry->gnuMaskIdxBits = bitMaskWords - 1;
		entry->gnuShift = *hash++;
		
		entry->gnuBitMask = hash;

		hash += _ELF_NATIVE_CLASS / 32 * bitMaskWords;
		
		entry->gnuBuckets = hash;	
		hash += entry->numBuckets;
		entry->gnuChainZero = hash - symBias;
			
	}else if (entry->dynamicInfo[DT_HASH])
	{
		hashAddr=(ElfSymIndex*)entry->dynamicInfo[DT_HASH];
		entry->numBuckets=*hashAddr++;
		entry->elfBuckets=++hashAddr;
		hashAddr+=entry->numBuckets;
		entry->elfChains=hashAddr;
	}else{
		link_puts("No hash table found. Exiting\n");
		SysExit(0);
	}
}
