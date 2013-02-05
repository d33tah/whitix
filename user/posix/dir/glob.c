#include <errno.h>
#include <glob.h>
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>

int glob_in_dir(const char* pattern, const char* directory, int flags,
	int (*errFunc)(const char*, int), glob_t* pGlob)
{
	int numFound = 0;
	DIR* dir = NULL;
	size_t length;
	struct dirent* ent;
	
	struct GlobEntry
	{
		struct GlobEntry* next;
		char* name;
	};
	
	struct GlobEntry* head=NULL;

	dir = (flags & GLOB_ALTDIRFUNC) ? pGlob->gl_opendir(directory)
		: opendir(directory);
	
	if (!dir)
	{
		if (((errno != ENOTDIR) && (errFunc && errFunc(directory, errno)))
			|| (flags & GLOB_ERR))
			return GLOB_ABORTED;
	}else{
		
		while (1)
		{
			ent = (flags & GLOB_ALTDIRFUNC) ? pGlob->gl_readdir(dir)
				: readdir(dir);
			
			if (!ent)
				break;

//			printf("%s %s\n", ent->d_name, pattern);
		
			if (ent->d_name[0] == '.')
				continue;
	
			if ( ( pattern[0] == '.' && strcasestr(ent->d_name, pattern) ) 
				|| !strcasecmp(ent->d_name, pattern))
			{
//				printf("Found %s in %s\n", pattern, ent->d_name);
			
				/* Allocate new glob entry. */
				struct GlobEntry* entry = (struct GlobEntry*)alloca(sizeof(struct GlobEntry));
			
				entry->name = strdup(ent->d_name);
				entry->next = NULL;
			
				if (!head)
					head = entry;
				else{
					struct GlobEntry* curr=head;
				
					while (curr->next)
						curr = curr->next;
					
					curr->next=entry;
				}
			
				numFound++;
			}
		}
	}

	if (dir)
	{			
		if (flags & GLOB_ALTDIRFUNC)
			pGlob->gl_closedir(dir);
		else
			closedir(dir);
	}
	
	if (numFound == 0 && (flags & GLOB_NOCHECK))
	{
		numFound = 1;
		head = (struct GlobEntry*)alloca(sizeof(struct GlobEntry));
		head->next = NULL;
		head->name = strdup(pattern);
	}
		
	if (numFound > 0)
	{
		pGlob->gl_pathv = realloc(pGlob->gl_pathv,
		(pGlob->gl_pathc + pGlob->gl_offs + numFound + 1) * sizeof (char *));
		
		struct GlobEntry* curr = head;
		
		while (curr)
		{
			pGlob->gl_pathv[pGlob->gl_offs + pGlob->gl_pathc++] = curr->name;
			curr = curr->next;
		}
		
		pGlob->gl_pathv[pGlob->gl_offs + pGlob->gl_pathc] = NULL;
	}
		
	return (numFound == 0) ? GLOB_NOMATCH : 0;
}

int NameCompare(const void* a, const void* b)
{
	const char* s1 = (const char*)a;
	const char* s2 = (const char*)b;
	
	return strcmp(s1, s2);
}

int glob_pattern_p(char* name, int flag)
{
	return 0;
}

int GlobPrefixArray(char* dirName, char** array, size_t n)
{
	int i;
	size_t dirLen = strlen(dirName);
	
	if (dirLen == 1 && dirName[0] == '/')
		dirLen = 0;
	
	for (i=0; i<n; i++)
	{
		size_t eltLen = strlen(array[i]) + 1;
		char* new = (char*) malloc(dirLen + eltLen + 1);
		
		if (!new)
		{
			while (i > 0)
				free(array[--i]);
			return 1;
		}
		
		strncpy(new, dirName, dirLen);
		new[dirLen]='\0';
		strcat(new, "/");
		strcat(new, array[i]);
		
//		printf("%s -> %s\n", array[i], new);
		
		free(array[i]);
		array[i] = new;
	}
	
	return 0;
}

int glob(const char* pattern, int flags, int (*errFunc)(const char* ePath, int eerrno),
	glob_t* pGlob)
{
	char* fileName, *dirName;
	int dirLen;
	size_t oldCount;
	int status;
	
//	printf("glob(%s, %d, %#X, %#X)\n", pattern, flags, errFunc, pGlob);
	
	if (!pattern || !pGlob || (flags & ~_GLOB_FLAGS))
	{
		errno = EINVAL;
		return -1;
	}
	
	if (!(flags & GLOB_DOOFFS))
		pGlob->gl_offs = 0;
		
	fileName = strrchr(pattern, '/');
	
	if (!fileName)
	{
		/* TODO: tilde check? */
		
		fileName = pattern;
		dirName = ".";
		dirLen = 0;
	}else if (fileName == pattern)
	{
		dirName = "/";
		dirLen = 1;
		fileName++;
	}else{
		char* new;
		
		dirLen = fileName - pattern;
		new = alloca(dirLen + 1);
		
		strncpy(new, pattern, dirLen);
		new[dirLen]='\0';
		dirName = new;
		
		fileName++;
		
//		printf("fileName = '%s', pattern = '%s', dirName = '%s'\n", fileName, pattern, dirName);
		
		if (!fileName[0] && dirLen > 1)
		{
			int val = glob (dirName, flags | GLOB_MARK, errFunc, pGlob);
			
//			if (val == 0)
//				pGlob->gl_flags = ((pGlob->gl_flags & ~GLOB_MARK)
//			       | (flags & GLOB_MARK));
	  		return val;
		}
	}
	
	if (!(flags & GLOB_APPEND))
	{
		pGlob->gl_pathc = 0;
		
		if (!(flags & GLOB_DOOFFS))
			pGlob->gl_pathv = NULL;
		else{
			printf("DOOFFS\n");
		}
	}
	
	if (glob_pattern_p(dirName, !(flags & GLOB_NOESCAPE)))
	{
		printf("glob pattern\n");
		exit(0);
	}else{
		oldCount = pGlob->gl_pathc + pGlob->gl_offs;

		status = glob_in_dir(fileName, dirName, flags, errFunc, pGlob);
	
		if (status)
			return status;
			
		if (dirLen)
		{
			if (GlobPrefixArray(dirName, &pGlob->gl_pathv[oldCount + pGlob->gl_offs],
			    pGlob->gl_pathc - oldCount))
			{
				printf("TODO: Prefix failed\n");
				exit(0);
			}
		}
	}
	
	if (flags & GLOB_MARK)
	{
		/* Append slashes to directory names. */
		size_t i;
		struct stat st;
		
		for (i = oldCount; i < pGlob->gl_pathc + pGlob->gl_offs; ++i)
		{
			if (!pGlob->gl_stat(pGlob->gl_pathv[i], &st)
		&& S_ISDIR (st.st_mode))
			{
				size_t len = strlen (pGlob->gl_pathv[i]) + 2;
				char *new = realloc (pGlob->gl_pathv[i], len);
				
				if (!new)
				{
					globfree (pGlob);
					pGlob->gl_pathc = 0;
					return GLOB_NOSPACE;
				}
				
				strcpy (&new[len - 2], "/");
				pGlob->gl_pathv[i] = new;
			}
		}
	}
	
	if (!(flags & GLOB_NOSORT))
	{
		qsort(&pGlob->gl_pathv[oldCount], pGlob->gl_pathc + pGlob->gl_offs - oldCount,
			sizeof(char*), NameCompare);
	}
	
	return 0;
}

void globfree(glob_t* pGlob)
{
	if (pGlob->gl_pathv)
	{
		int i;
		
		for (i=0; i<pGlob->gl_pathc; i++)
		{
			if (pGlob->gl_pathv[pGlob->gl_offs + i])
				free(pGlob->gl_pathv[pGlob->gl_offs + i]);
		}
		
		free(pGlob->gl_pathv);
		pGlob->gl_pathv = NULL;
	}
}

# if defined (STDC_HEADERS) || !defined (isascii)
#  define ISASCII(c) 1
# else
#  define ISASCII(c) isascii(c)
# endif

# define ISUPPER(c) (ISASCII (c) && isupper (c))


/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, nonzero if not.  */
int fnmatch(const char *pattern, const char *string, int flags)
{
	register const char *p = pattern, *n = string;
	register char c;

/* Note that this evaluates C many times.  */
# define FOLD(c) ((flags & FNM_CASEFOLD) && ISUPPER (c) ? tolower (c) : (c))

	while ((c = *p++) != '\0') {
		c = FOLD(c);

		switch (c) {
		case '?':
			if (*n == '\0')
				return FNM_NOMATCH;
			else if ((flags & FNM_FILE_NAME) && *n == '/')
				return FNM_NOMATCH;
			else if ((flags & FNM_PERIOD) && *n == '.' &&
					 (n == string
					  || ((flags & FNM_FILE_NAME)
						  && n[-1] == '/'))) return FNM_NOMATCH;
			break;

		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				c = *p++;
				if (c == '\0')
					/* Trailing \ loses.  */
					return FNM_NOMATCH;
				c = FOLD(c);
			}
			if (FOLD(*n) != c)
				return FNM_NOMATCH;
			break;

		case '*':
			if ((flags & FNM_PERIOD) && *n == '.' &&
				(n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
				return FNM_NOMATCH;

			for (c = *p++; c == '?' || c == '*'; c = *p++) {
				if ((flags & FNM_FILE_NAME) && *n == '/')
					/* A slash does not match a wildcard under FNM_FILE_NAME.  */
					return FNM_NOMATCH;
				else if (c == '?') {
					/* A ? needs to match one character.  */
					if (*n == '\0')
						/* There isn't another character; no match.  */
						return FNM_NOMATCH;
					else
						/* One character of the string is consumed in matching
						   this ? wildcard, so *??? won't match if there are
						   less than three characters.  */
						++n;
				}
			}

			if (c == '\0')
				return 0;

			{
				char c1 = (!(flags & FNM_NOESCAPE) && c == '\\') ? *p : c;

				c1 = FOLD(c1);
				for (--p; *n != '\0'; ++n)
					if ((c == '[' || FOLD(*n) == c1) &&
						fnmatch(p, n, flags & ~FNM_PERIOD) == 0)
						return 0;
				return FNM_NOMATCH;
			}

		case '[':
		{
			/* Nonzero if the sense of the character class is inverted.  */
			register int not;

			if (*n == '\0')
				return FNM_NOMATCH;

			if ((flags & FNM_PERIOD) && *n == '.' &&
				(n == string || ((flags & FNM_FILE_NAME) && n[-1] == '/')))
				return FNM_NOMATCH;

			not = (*p == '!' || *p == '^');
			if (not)
				++p;

			c = *p++;
			for (;;) {
				register char cstart = c, cend = c;

				if (!(flags & FNM_NOESCAPE) && c == '\\') {
					if (*p == '\0')
						return FNM_NOMATCH;
					cstart = cend = *p++;
				}

				cstart = cend = FOLD(cstart);

				if (c == '\0')
					/* [ (unterminated) loses.  */
					return FNM_NOMATCH;

				c = *p++;
				c = FOLD(c);

				if ((flags & FNM_FILE_NAME) && c == '/')
					/* [/] can never match.  */
					return FNM_NOMATCH;

				if (c == '-' && *p != ']') {
					cend = *p++;
					if (!(flags & FNM_NOESCAPE) && cend == '\\')
						cend = *p++;
					if (cend == '\0')
						return FNM_NOMATCH;
					cend = FOLD(cend);

					c = *p++;
				}

				if (FOLD(*n) >= cstart && FOLD(*n) <= cend)
					goto matched;

				if (c == ']')
					break;
			}
			if (!not)
				return FNM_NOMATCH;
			break;

		  matched:;
			/* Skip the rest of the [...] that already matched.  */
			while (c != ']') {
				if (c == '\0')
					/* [... (unterminated) loses.  */
					return FNM_NOMATCH;

				c = *p++;
				if (!(flags & FNM_NOESCAPE) && c == '\\') {
					if (*p == '\0')
						return FNM_NOMATCH;
					/* XXX 1003.2d11 is unclear if this is right.  */
					++p;
				}
			}
			if (not)
				return FNM_NOMATCH;
		}
			break;

		default:
			if (c != FOLD(*n))
				return FNM_NOMATCH;
		}

		++n;
	}

	if (*n == '\0')
		return 0;

	if ((flags & FNM_LEADING_DIR) && *n == '/')
		/* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
		return 0;

	return FNM_NOMATCH;

# undef FOLD
}
