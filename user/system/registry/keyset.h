#ifndef SERVER_KEYSET_H
#define SERVER_KEYSET_H

#include "key.h"

/* STRUCT RegKeySet:
 *
 * Internal representation of a keyset. A keyset is effectively a directory in
 * the registry filesystem, storing "files" (RegKeys) and child directories
 * (RegKeySets)
 */

struct RegKeySet
{
	char* name;
	struct RegKeySet* children;
	int numChildren;
	struct RegKey* keys;
	int numKeys;
	struct RegKeySet* next;
};

/* Functions to manage keysets. */
void RegKeySetCreate(struct RegKeySet* keySet, char* path);
struct RegKeySet* RegKeySetGet(char* path);
struct RegKeySet* RegKeySetCreateOne(struct RegKeySet* parent, char* name, int length);

/* Keyset root functions. */
int RegKeySetCreateRoot();
struct RegKeySet* RegKeySetGetRoot();

#endif

